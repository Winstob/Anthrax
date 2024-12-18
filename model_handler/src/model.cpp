/* ---------------------------------------------------------------- *\
 * model.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-12-12
\* ---------------------------------------------------------------- */

#include "model.hpp"

#include <iostream>

namespace Anthrax
{

Model::Model(size_t size_x, size_t size_y, size_t size_z)
{
	// octrees are always a cube, so find the maximum axis length
	size_t axis_size = size_x;
	if (size_y > axis_size) axis_size = size_y;
	if (size_z > axis_size) axis_size = size_z;
	// calculate the number of layers needed for the octree
	unsigned int num_layers = 0;
	if (axis_size != 0)
	{
		while (((axis_size-1) >> num_layers) >= 1)
		{
			num_layers++;
		}
	}

	octree_ = new Octree(num_layers);
	return;
}


Model::~Model()
{
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


void Model::addToWorld(World *world, unsigned int x, unsigned int y, unsigned int z)
{
	if (!octree_)
	{
		throw std::runtime_error("Model::addToWorld(): octree not initialized!");
	}
	octree_->addToWorld(world, x, y, z);
	return;
}

} // namespace Anthrax
