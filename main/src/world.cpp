/* ---------------------------------------------------------------- *\
 * world.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-08-08
\* ---------------------------------------------------------------- */
// TODO: fix differences between uniformity pool types between CPU and GPU (may cause issues on big endian systems?)
#include "world.hpp"

#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <cmath>

#include "vox_handler.hpp"
#include "gltf_handler.hpp"

namespace Anthrax
{

World::World(int num_layers, Device gpu_device)
{
	device_ = gpu_device;
	mainSetup(num_layers);
	return;
}


World::World(int num_layers)
{
	mainSetup(num_layers);
	return;
}


void World::mainSetup(int num_layers)
{
	std::cout << num_layers << std::endl;
	if (num_layers < 1)
	{
		throw std::runtime_error("World must have at least 1 layer!");
	}
	octree_ = new Octree(num_layers);
	generate();
}


World::~World()
{
	delete octree_;
}


World& World::operator=(const World& other)
{
	throw std::runtime_error("Don't use this (World::operator=)");
	octree_ = other.octree_;
	device_ = other.device_;
	return *this;
}


void World::generate()
{
	std::cout << "generating world... " << std::flush;

	// set up world as empty node
	octree_->clear();
	materials_[0] = Material(0.0, 0.0, 0.0, 0.0);
	

	/*
	// vox file
	VoxHandler voxfile_parser(this);
	Material *materials_ptr = voxfile_parser.getMaterialsPtr();
	for (unsigned int i = 0; i < 256; i++)
	{
		materials_[i] = materials_ptr[i];
	}
	*/

	// gltf file
	/*
	GltfHandler gltf_handler;
	Voxelizer voxelizer(gltf_handler.getMeshPtr(), device_);
	Model *model = voxelizer.createModel();
	unsigned int offset[3] = { 2048, 2048, 2048 };
	Quaternion rot(0.9238795325, 0.33, 0.3826834324, 0.1);
	//Quaternion rot(0.23, 0.2341, 0.552, 0.388);
	rot.normalize();
	model->rotate(rot);
	//model->rotate(Quaternion(1.0, 0.0, 0.0, 0.0)); // should turn it upside down?
	//model->addToWorld(this, offset[0], offset[1], offset[2]);
	delete model;
	Material *materials = voxelizer.getMaterials();
	for (unsigned int i = 0; i < voxelizer.getNumMaterials(); i++)
	{
		materials_[i] = materials[i];
	}
	materials_[0] = Material(0.0, 0.0, 0.0, 0.0);
	*/


	//std::cout << "World size: " << (next_available_pool_index_>>(18-(LOG2K*3))) << "MB" << std::endl; // *4(four bytes per 32-bit word), <<(LOG2K*3)(2^(log2k*3) children per index), >>20(B->MB)
	
	/*
	// Serpinski pyramid -- store all voxels individually
	unsigned int next_index = 1;
	unsigned int num_indices = 1;
	for (unsigned int i = 1; i < num_layers_; i++)
	{
		num_indices = num_indices*64 + 1;
	}
	for (unsigned int j = 0; j < num_indices; j++)
	{

		bool it, iit, iiit;
		for (unsigned int i = 0; i < 8; i++)
		{
			if (i == 0 || i == 3 || i == 5 || i == 6) it = true;
			else it = false;
			unsigned int ix = (i & 1);
			unsigned int iy = (i & 2) >> 1;
			unsigned int iz = (i & 4) >> 2;
			for (unsigned int ii = 0; ii < 8; ii++)
			{
				if (ii == 0 || ii == 3 || ii == 5 || ii == 6) iit = true;
				else iit = false;
				unsigned int iix = (ii & 1);
				unsigned int iiy = (ii & 2) >> 1;
				unsigned int iiz = (ii & 4) >> 2;
				for (unsigned int iii = 0; iii < 8; iii++)
				{
					if (iii == 0 || iii == 3 || iii == 5 || iii == 6) iiit = true;
					else iiit = false;
					unsigned int iiix = (iii & 1);
					unsigned int iiiy = (iii & 2) >> 1;
					unsigned int iiiz = (iii & 4) >> 2;

					unsigned int x = (ix << 2) | (iix << 1) | (iiix);
					unsigned int y = (iy << 2) | (iiy << 1) | (iiiy);
					unsigned int z = (iz << 2) | (iiz << 1) | (iiiz);

					unsigned int index = (z << 6) | (y << 3) | (x);
					index |= (j << 9);
					unsigned int uniformity_pool_index = index >> 3;
					char uniformity_pool_index_bit = 1 << (index & 7);
					if (it && iit && iiit)
					{
						indirection_pool_[index] = next_index;
						uniformity_pool_[uniformity_pool_index] &= !uniformity_pool_index_bit;
						voxel_type_pool_[index] = 1;
						next_index++;
					}
					else
					{
						indirection_pool_[index] = 0;
						uniformity_pool_[uniformity_pool_index] |= uniformity_pool_index_bit;
						voxel_type_pool_[index] = 0;
					}
				}
			}
		}

	}
	*/
	/*
	// Serpinski pyramid -- store only two voxel tree elements that refer to themselves
	for (unsigned int j = 0; j < 2; j++)
	{

		bool it, iit, iiit;
		for (unsigned int i = 0; i < 8; i++)
		{
			if (i == 0 || i == 3 || i == 5 || i == 6) it = true;
			else it = false;
			unsigned int ix = (i & 1);
			unsigned int iy = (i & 2) >> 1;
			unsigned int iz = (i & 4) >> 2;
			for (unsigned int ii = 0; ii < 8; ii++)
			{
				if (ii == 0 || ii == 3 || ii == 5 || ii == 6) iit = true;
				else iit = false;
				unsigned int iix = (ii & 1);
				unsigned int iiy = (ii & 2) >> 1;
				unsigned int iiz = (ii & 4) >> 2;
				for (unsigned int iii = 0; iii < 8; iii++)
				{
					if (iii == 0 || iii == 3 || iii == 5 || iii == 6) iiit = true;
					else iiit = false;
					unsigned int iiix = (iii & 1);
					unsigned int iiiy = (iii & 2) >> 1;
					unsigned int iiiz = (iii & 4) >> 2;

					unsigned int x = (ix << 2) | (iix << 1) | (iiix);
					unsigned int y = (iy << 2) | (iiy << 1) | (iiiy);
					unsigned int z = (iz << 2) | (iiz << 1) | (iiiz);

					unsigned int index = (z << 6) | (y << 3) | (x);
					index |= (j << 9);
					unsigned int uniformity_pool_index = index >> 3;
					char uniformity_pool_index_bit = 1 << (index & 7);
					if (it && iit && iiit)
					{
						indirection_pool_[index] = 1;
						uniformity_pool_[uniformity_pool_index] &= !uniformity_pool_index_bit;
						voxel_type_pool_[index] = 1;
					}
					else
					{
						indirection_pool_[index] = 0;
						uniformity_pool_[uniformity_pool_index] |= uniformity_pool_index_bit;
						voxel_type_pool_[index] = 0;
					}
				}
			}
		}

	}
	*/
	/*
	for (unsigned int i = 0; i < 2; i++)
	{
		generateSerpinskiPyramidNode(i);
	}
	materials_[1] = Material(1.0, 0.4, 0.8);
	*/
	std::cout << "done!" << std::endl;
	return;
}


void World::generateSerpinskiPyramidNode(unsigned int index)
{
	generateSingleSerpinskiPyramidNode(index, LOG2K, LOG2K, 0, 0, 0, false);
	return;
}


void World::generateSingleSerpinskiPyramidNode(unsigned int node_index, int num_layers, int layer, unsigned int x, unsigned int y, unsigned int z, bool is_air)
{
	/*
	if (layer == 0)
	{
		unsigned int secondary_index = (z << (2*num_layers)) | (y << num_layers) | x;

		if (is_air)
		{
			setIndirection(node_index, secondary_index, 0);
			setUniformity(node_index, secondary_index, 1);
			setVoxelType(node_index, secondary_index, 0);
		}
		else
		{
			setIndirection(node_index, secondary_index, 1);
			setUniformity(node_index, secondary_index, 0);
			setVoxelType(node_index, secondary_index, 1);
		}
		return;
	}

	for (unsigned int i = 0; i < 8; i++)
	{
		unsigned int ix = (i & 1);
		unsigned int iy = (i & 2) >> 1;
		unsigned int iz = (i & 4) >> 2;
		unsigned int new_x = (x << 1) | ix;
		unsigned int new_y = (y << 1) | iy;
		unsigned int new_z = (z << 1) | iz;
		if (!is_air && (i == 0 || i == 3 || i == 5 || i == 6))
		{
			generateSingleSerpinskiPyramidNode(node_index, num_layers, layer-1, new_x, new_y, new_z, false);
		}
		else
		{
			generateSingleSerpinskiPyramidNode(node_index, num_layers, layer-1, new_x, new_y, new_z, true);
		}
	}
	return;
	*/
}


void World::addModel(
		Model *model,
		int32_t x_offset,
		int32_t y_offset,
		int32_t z_offset
		)
{
	octree_->mergeOctree(model->getOctree(), x_offset, y_offset, z_offset);
	return;
}


} // namespace Anthrax
