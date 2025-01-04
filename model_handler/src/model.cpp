/* ---------------------------------------------------------------- *\
 * model.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-12-12
\* ---------------------------------------------------------------- */

#include "model.hpp"

#include <iostream>
#include <vector>

namespace Anthrax
{

Model::Model(size_t size_x, size_t size_y, size_t size_z, Device gpu_device)
{
	mainSetup(size_x, size_y, size_z);
	use_gpu_device_ = true;

	device_ = gpu_device;
	rotationComputeShaderSetup();
	return;
}


Model::Model(size_t size_x, size_t size_y, size_t size_z)
{
	mainSetup(size_x, size_y, size_z);
	use_gpu_device_ = false;
	return;
}


void Model::mainSetup(size_t size_x, size_t size_y, size_t size_z)
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
	size_ = 1u << num_layers;

	octree_ = new Octree(num_layers);
	current_rotation_ = Quaternion();
	return;
}


Model::~Model()
{
	if (octree_)
	{
		delete octree_;
		octree_ = nullptr;
	}
	if (use_gpu_device_)
	{
		rotation_shader_manager_.destroy();
		for (unsigned int i = 0; i < rotation_shader_descriptors_.size(); i++)
		{
			rotation_shader_descriptors_[i].destroy();
		}
		rotation_input_buffer_.destroy();
		rotation_output_buffer_.destroy();
		// the device wasn't have been created by the Model, so it shouldn't be destroyed here
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
	Timer timer(Timer::MILLISECONDS);
	timer.start();
	// TODO: not naive approach - possibly use compute shader?
	int min = -(size_>>1);
	int max = min + size_;
	Octree *rotated_octree = new Octree(octree_->getLayer());

	std::vector<float> old_angles = current_rotation_.eulerAngles();
	std::vector<float> new_angles = quat.eulerAngles();

	int num_voxels = 0;

	// calculate shear angles
	for (int z = min; z < max; z++)
	{
		for (int y = min; y < max; y++)
		{
			for (int x = min; x < max; x++)
			{
				uint16_t voxel_type = octree_->getVoxel(x, y, z);
				if (voxel_type == 0)
					continue;
				num_voxels++;
				int new_x = x;
				int new_y = y;
				int new_z = z;

				// undo the current rotation
				//rotateVoxel(&new_x, &new_y, &new_z, -old_angles[0], -old_angles[1], -old_angles[2]);
				//rotateVoxel(&new_z, &new_y, &new_x, old_angles[2], old_angles[1], old_angles[0]);
				//unrotateVoxel(&new_x, &new_y, &new_z, old_angles[0], old_angles[1], old_angles[2]);
				unrotateVoxelRoll(&new_x, &new_y, &new_z, old_angles[2]);
				unrotateVoxelPitch(&new_x, &new_y, &new_z, old_angles[1]);
				unrotateVoxelYaw(&new_x, &new_y, &new_z, old_angles[0]);
				
				// do the new rotation
				//rotateVoxel(&new_x, &new_y, &new_z, new_angles[0], new_angles[1], new_angles[2]);
				rotateVoxelYaw(&new_x, &new_y, &new_z, new_angles[0]);
				rotateVoxelPitch(&new_x, &new_y, &new_z, new_angles[1]);
				rotateVoxelRoll(&new_x, &new_y, &new_z, new_angles[2]);

				rotated_octree->setVoxel(new_x, new_y, new_z, voxel_type);
			}
		}
	}
	delete octree_;
	octree_ = rotated_octree;
	current_rotation_ = quat;
	std::cout << "Time to rotate: " << timer.stop() << "ms" << std::endl;
	return;
	/*
	// future implementation: use compute shader for rotation
	// split up the model into MODEL_ROTATION_CHUNK_SIZE^3 cubes
	float chunk_inclusion_zone_radius = 0.57; // approximately 1/sqrt(3) rounded down.
																						// chunks need to overlap because some
																						// rotations may exceed the chunk boundaries.
	//int num_chunks_per_axis = static_cast<int>(ceil(size_/MODEL_ROTATION_CHUNK_SIZE));
	int num_chunks_per_axis = 1;
	int test_model_size = size_ - MODEL_ROTATION_CHUNK_SIZE;
	while (test_model_size > 0)
	{
		test_model_size -= static_cast<int>(floor(MODEL_ROTATION_CHUNK_SIZE*chunk_inclusion_zone_radius));
		num_chunks_per_axis++;
	}
	std::cout << size_ << std::endl;
	int chunk_offset = static_cast<int>(floor(MODEL_ROTATION_CHUNK_SIZE*chunk_inclusion_zone_radius));
	for (int chunk_z = 0; chunk_z < num_chunks_per_axis; chunk_z++)
	{
		for (int chunk_y = 0; chunk_y < num_chunks_per_axis; chunk_y++)
		{
			for (int chunk_x = 0; chunk_x < num_chunks_per_axis; chunk_x++)
			{
				std::cout << "Rotating chunk |" << chunk_x << "|" << chunk_y << "|" << chunk_z << "|" << std::endl;
				int chunk_min[3] = { chunk_x*chunk_offset, chunk_y*chunk_offset, chunk_z*chunk_offset };
				int chunk_max[3];
				for (unsigned int axis = 0; axis < 3; axis++)
				{
					chunk_min[axis] -= size_ >> 1; // need to move center of chunk to 0,0,0
					chunk_max[axis] = chunk_min[axis]+MODEL_ROTATION_CHUNK_SIZE;
				}
				// first rotate the chunk around its center
				// then find the center's correct location (rotate the chunk center around the model enter)
				// translate the chunk
			}
		}
	}
	*/
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

void Model::addToWorld(World *world, unsigned int x, unsigned int y, unsigned int z)
{
	if (!octree_)
	{
		throw std::runtime_error("Model::addToWorld(): octree not initialized!");
	}
	octree_->addToWorld(world, x, y, z);
	return;
}


void Model::rotationComputeShaderSetup()
{
	rotation_shader_manager_ = ComputeShaderManager(device_, std::string(xstr(SHADER_DIRECTORY)) + "model_rotation_c.spv");

	// set up buffers
	rotation_input_buffer_ = Buffer(
			device_,
			MODEL_ROTATION_CHUNK_SIZE*MODEL_ROTATION_CHUNK_SIZE*MODEL_ROTATION_CHUNK_SIZE*2, // 2 bytes per voxel
			Buffer::STORAGE_TYPE,
			0,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
	rotation_output_buffer_ = Buffer(
			device_,
			MODEL_ROTATION_CHUNK_SIZE*MODEL_ROTATION_CHUNK_SIZE*MODEL_ROTATION_CHUNK_SIZE*2, // 2 bytes per voxel
			Buffer::STORAGE_TYPE,
			0,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);


	// set up descriptors
	std::vector<Buffer> buffers;
	std::vector<Image> images;
	buffers.clear();
	images.clear();
	buffers.push_back(rotation_input_buffer_);
	buffers.push_back(rotation_output_buffer_);
	rotation_shader_descriptors_.clear();
	rotation_shader_descriptors_.push_back(Descriptor(device_, Descriptor::ShaderStage::COMPUTE, buffers, images));
	rotation_shader_manager_.setDescriptors(rotation_shader_descriptors_);
	
	// initialize the shader module
	//rotation_shader_manager_.init();
	return;
}

} // namespace Anthrax
