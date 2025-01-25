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

Model::Model(size_t size_x, size_t size_y, size_t size_z)
{
	mainSetup(size_x, size_y, size_z);
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

	original_octree_ = new Octree(num_layers);
	octree_ = new Octree(num_layers);
	current_rotation_ = Quaternion();
	frame_rotation_timer_ = Timer(Timer::MICROSECONDS);
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
	throw std::runtime_error("Model::copy() not yet implemented!");
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
	current_rotation_ = quat;
	octree_->clear();
	// rotate breadth-first: rotate highest layer,
	// then work down to layer 0
	rotation_status_.in_progress = true;
	rotation_status_.lowest_completed_layer = original_octree_->getLayer()+1;

	continueRotation();

	return;
}


void Model::continueRotation()
{
	rotation_status_.timeout = 500;
	if (!rotation_status_.in_progress)
	{
		return;
	}

	// the first rotateOnLayer must be a continuation unless we just started the
	// rotation
	// this assumes that the first layer will successfully be completely rotated
	// within the span of a single frame, which is hopefully reasonable since
	// it is literally a single voxel
	bool continuation =
			(rotation_status_.lowest_completed_layer <= original_octree_->getLayer());

	frame_rotation_timer_.start();
	octree_->setSplitMode(Octree::SPLIT_MODE_AIRFILL);
	while (rotation_status_.lowest_completed_layer > 0)
	{
		if (!rotateOnLayer(current_rotation_,
				rotation_status_.lowest_completed_layer-1,
				continuation))
		{
			break;
		}
		continuation = false;
		rotation_status_.lowest_completed_layer--;
	}
	octree_->setSplitMode(Octree::SPLIT_MODE_NORMAL);
	frame_rotation_timer_.stop();

	if (rotation_status_.lowest_completed_layer == 0)
	{
		rotation_status_.in_progress = false;
	}

	return;
}


bool Model::rotateOnLayer(Quaternion quat, int layer, bool continuation)
{
	if (layer >= original_octree_->getLayer())
	{
		return true;
	}

	// we don't really need to check the timer after every voxel since it'll just
	// slow the whole thing down past a certain point. Only check the time once
	// every <time_check_interval> voxels.
	int time_check_interval = 1000;

	int min = -(1u<<(original_octree_->getLayer()-layer-1));
	int max = (1u<<(original_octree_->getLayer()-layer-1)) - 1;

	int x_min, y_min, z_min;
	if (continuation)
	{
		x_min = rotation_status_.next_x;
		y_min = rotation_status_.next_y;
		z_min = rotation_status_.next_z;
	}
	else
	{
		x_min = min;
		y_min = min;
		z_min = min;
	}

	std::vector<float> angles = quat.eulerAngles();

	int num_voxels = 0;

	for (int z = z_min; z <= max; z++)
	{
		for (int y = y_min; y <= max; y++)
		{
			for (int x = x_min; x <= max; x++)
			{
				if (rotation_parallelization_method_ ==
						ROTATION_PARALLELIZATION_TIME_ALLOCATED &&
						num_voxels % time_check_interval == 0)
				{
					if (frame_rotation_timer_.query() >=
							rotation_status_.timeout)
					{
						rotation_status_.next_x = x;
						rotation_status_.next_y = y;
						rotation_status_.next_z = z;
						return false;
					}
				}

				uint16_t voxel_type = original_octree_->getVoxelAtLayer(x, y, z, layer);
				num_voxels++;
				int new_x = x;
				int new_y = y;
				int new_z = z;

				rotateVoxelYaw(&new_x, &new_y, &new_z, angles[0]);
				rotateVoxelPitch(&new_x, &new_y, &new_z, angles[1]);
				rotateVoxelRoll(&new_x, &new_y, &new_z, angles[2]);

				if (new_x >= min && new_x <= max &&
						new_y >= min && new_y <= max &&
						new_z >= min && new_z <= max)
				{
					octree_->setVoxelAtLayer(new_x, new_y, new_z, voxel_type, layer);
				}
			}
			x_min = min;
		}
		y_min = min;
	}
	z_min = min;
	return true;
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

} // namespace Anthrax
