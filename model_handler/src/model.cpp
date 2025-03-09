/* ---------------------------------------------------------------- *\
 * model.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-12-12
\* ---------------------------------------------------------------- */
//TODO: destroy rotation shader stuff on exit

#include "model.hpp"

#include <iostream>
#include <vector>

namespace Anthrax
{

extern Device *anthrax_gpu;

Model::RotationStuff Model::rotation_stuff_;

Model::Model(size_t size_x, size_t size_y, size_t size_z)
{
	// since models can rotate, we need to set the octree size to
	// the maximum possible size when rotated (longest diagonal)
	size_t axis_size = static_cast<size_t>(ceil(sqrt((size_x*size_x) + (size_y*size_y) + (size_z*size_z))));
	// calculate the number of layers needed for the octree
	unsigned int num_layers = 0;
	if (axis_size != 0)
	{
		while (((axis_size-1) >> num_layers) >= 1)
		{
			num_layers++;
		}
	}
	octree_width_ = 1u << num_layers;

	original_octree_ = new Octree(num_layers);
	octree_ = new Octree(num_layers);
	current_rotation_ = Quaternion();

	// if needed, set up stuff necessary for gpu rotation
	rotation_stuff_.mutex.lock();
	if (!rotation_stuff_.initialized)
	{
		rotationStuffSetup();
	}
	rotation_stuff_.mutex.unlock();

	return;
}


Model::~Model()
{
	if (original_octree_)
	{
		delete original_octree_;
		original_octree_ = nullptr;
	}
	if (octree_)
	{
		delete octree_;
		octree_ = nullptr;
	}
	return;
}


void Model::copy(const Model& other)
{
	// TODO: this
	return;
}


void Model::setVoxel(int32_t x, int32_t y, int32_t z, uint16_t material_type)
{
	if (original_octree_)
	{
		original_octree_->setVoxel(x, y, z, material_type);
	}
	else
	{
		throw std::runtime_error("setVoxel(): original_octree member not yet initialized!");
	}

	if (octree_)
	{
		octree_->setVoxel(x, y, z, material_type);
	}
	else
	{
		throw std::runtime_error("setVoxel(): octree member not yet initialized!");
	}

	return;
}


void Model::rotate(Quaternion quat)
{
	rotateGPU(quat);
	return;
	current_rotation_ = quat;
	octree_->clear();
	//int precision = original_octree_->getLayer()-7;
	int precision = 0;
	// rotate breadth-first: rotate highest layer,
	// then work down to layer <precision>
	for (lowest_rotated_layer_ = original_octree_->getLayer()+1;
			lowest_rotated_layer_ > precision;
			lowest_rotated_layer_--)
	{
		octree_->setSplitMode(Octree::SPLIT_MODE_AIRFILL);
		rotateOnLayer(quat, lowest_rotated_layer_-1);
		octree_->setSplitMode(Octree::SPLIT_MODE_NORMAL);
	}
	return;
}


void Model::rotateOnLayer(Quaternion quat, int layer)
{
	if (layer >= original_octree_->getLayer())
	{
		return;
	}

	Timer timer(Timer::MILLISECONDS);
	timer.start();
	int min = -(1<<(original_octree_->getLayer()-layer-1));
	int max = (1<<(original_octree_->getLayer()-layer-1)) - 1;

	std::vector<float> angles = quat.eulerAngles();

	int num_voxels = 0;

	for (int z = min; z <= max; z++)
	{
		for (int y = min; y <= max; y++)
		{
			for (int x = min; x <= max; x++)
			{
				uint16_t voxel_type = original_octree_->getVoxelAtLayer(x, y, z, layer);
				num_voxels++;
				int new_x = x;
				int new_y = y;
				int new_z = z;

				rotateVoxelYaw(&new_x, &new_y, &new_z, angles[0]);
				rotateVoxelPitch(&new_x, &new_y, &new_z, angles[1]);
				rotateVoxelRoll(&new_x, &new_y, &new_z, angles[2]);

				if (new_x < min || new_x > max ||
						new_y < min || new_y > max ||
						new_z < min || new_z > max)
				{
					continue;
				}
				octree_->setVoxelAtLayer(new_x, new_y, new_z, voxel_type, layer);
			}
		}
	}
	std::cout << "Time to rotate layer " << layer << ": " << timer.stop()
			<< "ms" << std::endl;
	return;
}

/****************************************************************\
 * axis:
 *	0 = x (pitch)
 *	1 = y (yaw)
 *	2 = z (roll)
 * mode:
 *	0 = add (rotate)
 *	1 = subtract (un-rotate)
\****************************************************************/
void Model::rotateVoxelSingleAxis(int *x, int *y, int *z, float angle, int axis, int mode)
{
	float half_pi = PI/2.0;

	int primary_axis = axis;
	int secondary_axis = (axis + 1) % 3;
	int tertiary_axis = (axis + 2) % 3;

	float new_positions[3] = {
			static_cast<float>(*x),
			static_cast<float>(*y),
			static_cast<float>(*z),
	};
	for (unsigned int i = 0; i < 3; i++)
	{
		new_positions[i] += 0.5;
	}

	while (angle > PI) angle -= 2.0*PI;
	while (angle < -PI) angle += 2.0*PI;

	bool swap = false;
	if (angle > PI/2.0)
	{
		swap = true;
		//angle = PI - angle;
		angle -= PI;
	}
	if (angle < -PI/2.0)
	{
		swap = true;
		//angle = -PI - angle;
		angle += PI;
	}
	if (swap)
	{
		new_positions[secondary_axis] *= -1.0;
		new_positions[tertiary_axis] *= -1.0;
	}

	float secondary_shear = -tan(angle/2.0);
	float tertiary_shear = sin(angle);

	switch (mode)
	{
		case 0:
			new_positions[secondary_axis] += round(new_positions[tertiary_axis]*secondary_shear);
			new_positions[tertiary_axis] += round(new_positions[secondary_axis]*tertiary_shear);
			new_positions[secondary_axis] += round(new_positions[tertiary_axis]*secondary_shear);
			break;
		case 1:
			new_positions[secondary_axis] -= round(new_positions[tertiary_axis]*secondary_shear);
			new_positions[tertiary_axis] -= round(new_positions[secondary_axis]*tertiary_shear);
			new_positions[secondary_axis] -= round(new_positions[tertiary_axis]*secondary_shear);
			break;
	}

	*x = static_cast<int>(floor(new_positions[0]));
	*y = static_cast<int>(floor(new_positions[1]));
	*z = static_cast<int>(floor(new_positions[2]));
	return;
}

void Model::rotateVoxelYaw(int *x, int *y, int *z, float angle)
{
	rotateVoxelSingleAxis(x, y, z, angle, 1, 0);
}

void Model::rotateVoxelPitch(int *x, int *y, int *z, float angle)
{
	rotateVoxelSingleAxis(x, y, z, angle, 0, 0);
}

void Model::rotateVoxelRoll(int *x, int *y, int *z, float angle)
{
	rotateVoxelSingleAxis(x, y, z, angle, 2, 0);
}

void Model::unrotateVoxelYaw(int *x, int *y, int *z, float angle)
{
	rotateVoxelSingleAxis(x, y, z, angle, 1, 1);
}

void Model::unrotateVoxelPitch(int *x, int *y, int *z, float angle)
{
	rotateVoxelSingleAxis(x, y, z, angle, 0, 1);
}

void Model::unrotateVoxelRoll(int *x, int *y, int *z, float angle)
{
	rotateVoxelSingleAxis(x, y, z, angle, 2, 1);
}

void Model::rotateVoxel(int *x, int *y, int *z, float xangle, float yangle, float zangle)
{
	if (xangle > PI) xangle -= 2*PI;
	if (yangle > PI) yangle -= 2*PI;
	if (zangle > PI) zangle -= 2*PI;
	if (xangle < -PI) xangle += 2*PI;
	if (yangle < -PI) yangle += 2*PI;
	if (zangle < -PI) zangle += 2*PI;

	float x_y_shear = -tan(xangle/2.0);
	float x_z_shear = sin(xangle);
	float y_x_shear = -tan(yangle/2.0);
	float y_z_shear = sin(yangle);
	float z_x_shear = -tan(zangle/2.0);
	float z_y_shear = sin(zangle);

	float new_x = static_cast<float>(*x) + 0.5;
	float new_y = static_cast<float>(*y) + 0.5;
	float new_z = static_cast<float>(*z) + 0.5;

	new_y += round(new_z*x_y_shear);
	new_z += round(new_y*x_z_shear);
	new_y += round(new_z*x_y_shear);

	new_x += round(new_z*y_x_shear);
	new_z += round(new_x*y_z_shear);
	new_x += round(new_z*y_x_shear);

	new_x += round(new_y*z_x_shear);
	new_y += round(new_x*z_y_shear);
	new_x += round(new_y*z_x_shear);

	*x = static_cast<int>(floor(new_x));
	*y = static_cast<int>(floor(new_y));
	*z = static_cast<int>(floor(new_z));
	return;
}


void Model::unrotateVoxel(int *x, int *y, int *z, float xangle, float yangle, float zangle)
{
	if (xangle > PI) xangle -= 2*PI;
	if (yangle > PI) yangle -= 2*PI;
	if (zangle > PI) zangle -= 2*PI;
	if (xangle < -PI) xangle += 2*PI;
	if (yangle < -PI) yangle += 2*PI;
	if (zangle < -PI) zangle += 2*PI;

	float x_y_shear = -tan(xangle/2.0);
	float x_z_shear = sin(xangle);
	float y_x_shear = -tan(yangle/2.0);
	float y_z_shear = sin(yangle);
	float z_x_shear = -tan(zangle/2.0);
	float z_y_shear = sin(zangle);

	float new_x = static_cast<float>(*x) + 0.5;
	float new_y = static_cast<float>(*y) + 0.5;
	float new_z = static_cast<float>(*z) + 0.5;

	new_x -= round(new_y*z_x_shear);
	new_y -= round(new_x*z_y_shear);
	new_x -= round(new_y*z_x_shear);

	new_x -= round(new_z*y_x_shear);
	new_z -= round(new_x*y_z_shear);
	new_x -= round(new_z*y_x_shear);

	new_y -= round(new_z*x_y_shear);
	new_z -= round(new_y*x_z_shear);
	new_y -= round(new_z*x_y_shear);

	*x = static_cast<int>(floor(new_x));
	*y = static_cast<int>(floor(new_y));
	*z = static_cast<int>(floor(new_z));
	return;
}

/*
void Model::addToWorld(World *world, unsigned int x, unsigned int y, unsigned int z)
{
	if (!octree_)
	{
		throw std::runtime_error("Model::addToWorld(): octree not initialized!");
	}
	octree_->addToWorld(world, x, y, z);
	return;
}
*/

void Model::rotationStuffSetup()
{
	// TODO: clean all this stuff up at the end somehow
	rotation_stuff_.shader_manager = ComputeShaderManager(*anthrax_gpu,
			std::string(xstr(SHADER_DIRECTORY)) + "model_rotation_c.spv");
	rotation_stuff_.octree_rebuild_shader = ComputeShaderManager(*anthrax_gpu,
			std::string(xstr(SHADER_DIRECTORY)) + "octree_rebuild_c.spv");

	// allocate command pool
	rotation_stuff_.command_pool = anthrax_gpu->newCommandPool(Device::CommandType::COMPUTE);

	// allocate command buffer
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = rotation_stuff_.command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = 1;
	vkAllocateCommandBuffers(anthrax_gpu->logical, &alloc_info, &rotation_stuff_.command_buffer);

	// set up fence
	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	vkCreateFence(anthrax_gpu->logical, &fence_info, nullptr, &rotation_stuff_.fence);
	vkResetFences(anthrax_gpu->logical, 1, &rotation_stuff_.fence);

	rotation_stuff_.buffer_size = 64;
	recreateRotationShaders();

	return;
}

void Model::rotateGPU(Quaternion quat)
{
	std::lock_guard<std::mutex> guard(rotation_stuff_.mutex);
	// start at the lowest layer
	// - find the necessary sizes for the buffers
	// - set up buffers

	// The maximum number of *elements* in the octree pool is represented by the equation:
	//   \sum_{i=1}^{n}{2^{3i}}
	// which simplifies to:
	//   (2^{3n}-1)/7*8
	// where n represents the depth of the octree (width = 2^n). We must also multiply this
	// by the size of a single element to get the maximum size of the buffer.
	size_t necessary_buffer_size = ((1 << (3*octree_->getLayer())) - 1) / 7 * 8;
	necessary_buffer_size *= sizeof(Octree::OctreeNode);
	if (necessary_buffer_size > rotation_stuff_.buffer_size)
	{
		// initialize (or recreate) the shader module if needed
		rotation_stuff_.buffer_size = necessary_buffer_size;
		recreateRotationShaders();
		std::cout << "recreating buffers with size " << (necessary_buffer_size >> 20) << "MB (width " << octree_width_ << ")" << std::endl;
	}

	// copy octree over to gpu memory
	memcpy(rotation_stuff_.cpu_ssbo.getMappedPtr(), octree_->getOctreePool(), octree_->getOctreePoolSize());
	// TODO: add the rest of the stuff
	reinterpret_cast<RotationAngles*>(rotation_stuff_.rotation_angles_ubo.getMappedPtr())->old_angles
			= glm::make_vec3(old_rotation_.eulerAngles().data());
	reinterpret_cast<RotationAngles*>(rotation_stuff_.rotation_angles_ubo.getMappedPtr())->new_angles
			= glm::make_vec3(quat.eulerAngles().data());
	*(reinterpret_cast<uint32_t*>(rotation_stuff_.octree_depth_ubo.getMappedPtr()))
			= octree_->getLayer();

	// stage 1: populate lowest layer octree data
	rotation_stuff_.shader_manager.selectDescriptor(0);
	vkResetCommandBuffer(rotation_stuff_.command_buffer, 0);
	rotation_stuff_.shader_manager.recordCommandBuffer(rotation_stuff_.command_buffer, octree_width_, octree_width_, octree_width_);
	VkSubmitInfo compute_submit_info{};
	compute_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	compute_submit_info.commandBufferCount = 1;
	compute_submit_info.pCommandBuffers = &(rotation_stuff_.command_buffer);
	compute_submit_info.signalSemaphoreCount = 0;

	Timer timer;
	timer.start();
	vkQueueSubmit(anthrax_gpu->getComputeQueue(), 1, &compute_submit_info, rotation_stuff_.fence);
	vkWaitForFences(anthrax_gpu->logical, 1, &rotation_stuff_.fence, VK_TRUE, UINT64_MAX);
	vkResetFences(anthrax_gpu->logical, 1, &rotation_stuff_.fence);

	// stage 2: rebuild the higher layers of the octree
	rotation_stuff_.octree_rebuild_shader.selectDescriptor(0);
	// loop through higher layers until the root node is reached
	for (unsigned int layer = octree_->getLayer()-1; layer > 0; layer--)
	{
		// rebuild this layer of the octree
		*(reinterpret_cast<uint32_t*>(rotation_stuff_.octree_depth_ubo.getMappedPtr()))
				= layer;
		unsigned int layer_width = 1u << layer;
		vkResetCommandBuffer(rotation_stuff_.command_buffer, 0);
		rotation_stuff_.octree_rebuild_shader.recordCommandBuffer(rotation_stuff_.command_buffer, layer_width, layer_width, layer_width);
		VkSubmitInfo compute_submit_info{};
		compute_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		compute_submit_info.commandBufferCount = 1;
		compute_submit_info.pCommandBuffers = &(rotation_stuff_.command_buffer);
		compute_submit_info.signalSemaphoreCount = 0;
		vkQueueSubmit(anthrax_gpu->getComputeQueue(), 1, &compute_submit_info, rotation_stuff_.fence);
		vkWaitForFences(anthrax_gpu->logical, 1, &rotation_stuff_.fence, VK_TRUE, UINT64_MAX);
		vkResetFences(anthrax_gpu->logical, 1, &rotation_stuff_.fence);
	}
	
	// stage 3: defragment

	// copy octree data back to the octree member on the cpu

	std::cout << timer.stop() << std::endl;
	old_rotation_ = quat;
	return;
}


void Model::recreateRotationShaders()
{
	if (rotation_stuff_.shader_manager.initialized())
		rotation_stuff_.shader_descriptors[0].destroy();
	if (rotation_stuff_.octree_rebuild_shader.initialized())
		rotation_stuff_.octree_rebuild_descriptors[0].destroy();

	if (rotation_stuff_.cpu_ssbo.initialized())
		rotation_stuff_.cpu_ssbo.destroy();

	if (rotation_stuff_.gpu_ssbo.initialized())
		rotation_stuff_.gpu_ssbo.destroy();

	if (rotation_stuff_.freelist_ssbo.initialized())
		rotation_stuff_.freelist_ssbo.destroy();

	rotation_stuff_.cpu_ssbo = Buffer(
			*anthrax_gpu,
			rotation_stuff_.buffer_size,
			Buffer::STORAGE_TYPE,
			0,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
	rotation_stuff_.gpu_ssbo = Buffer(
			*anthrax_gpu,
			rotation_stuff_.buffer_size,
			Buffer::STORAGE_TYPE,
			0,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);
	rotation_stuff_.freelist_ssbo = Buffer(
			*anthrax_gpu,
			rotation_stuff_.buffer_size / sizeof(Octree::OctreeNode) / 8,
			Buffer::STORAGE_TYPE,
			0,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);
	// UBOs will never need to be recreated once created for the first time
	if (!rotation_stuff_.rotation_angles_ubo.initialized())
	{
		rotation_stuff_.rotation_angles_ubo = Buffer(
				*anthrax_gpu,
				sizeof(RotationAngles),
				Buffer::UNIFORM_TYPE,
				0,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
				);
	}
	if (!rotation_stuff_.octree_depth_ubo.initialized())
	{
		rotation_stuff_.octree_depth_ubo = Buffer(
				*anthrax_gpu,
				4,
				Buffer::UNIFORM_TYPE,
				0,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
				);
	}
	// set up stage 1 descriptors
	std::vector<Buffer> buffers;
	std::vector<Image> images;
	buffers.clear();
	images.clear();
	buffers.push_back(rotation_stuff_.cpu_ssbo);
	buffers.push_back(rotation_stuff_.gpu_ssbo);
	buffers.push_back(rotation_stuff_.freelist_ssbo);
	buffers.push_back(rotation_stuff_.rotation_angles_ubo);
	buffers.push_back(rotation_stuff_.octree_depth_ubo);
	rotation_stuff_.shader_descriptors.clear();
	rotation_stuff_.shader_descriptors.push_back(Descriptor(*anthrax_gpu,
			Descriptor::ShaderStage::COMPUTE, buffers, images));
	if (rotation_stuff_.shader_manager.initialized())
	{
		rotation_stuff_.shader_manager.updateDescriptors(rotation_stuff_.shader_descriptors);
	}
	else
	{
		rotation_stuff_.shader_manager.setDescriptors(rotation_stuff_.shader_descriptors);
		rotation_stuff_.shader_manager.init();
	}
	// set up stage 2 descriptors
	buffers.clear();
	images.clear();
	buffers.push_back(rotation_stuff_.gpu_ssbo);
	buffers.push_back(rotation_stuff_.freelist_ssbo);
	buffers.push_back(rotation_stuff_.octree_depth_ubo);
	rotation_stuff_.octree_rebuild_descriptors.clear();
	rotation_stuff_.octree_rebuild_descriptors.push_back(Descriptor(*anthrax_gpu,
			Descriptor::ShaderStage::COMPUTE, buffers, images));
	if (rotation_stuff_.octree_rebuild_shader.initialized())
	{
		rotation_stuff_.octree_rebuild_shader.updateDescriptors(rotation_stuff_.octree_rebuild_descriptors);
	}
	else
	{
		rotation_stuff_.octree_rebuild_shader.setDescriptors(rotation_stuff_.octree_rebuild_descriptors);
		rotation_stuff_.octree_rebuild_shader.init();
	}

	return;
}


} // namespace Anthrax
