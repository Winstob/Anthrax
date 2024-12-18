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

World::World(int num_layers)
{
	std::cout << num_layers << std::endl;
	if (num_layers < 1)
	{
		throw std::runtime_error("World cannot have zero levels!");
	}
	unsigned int num_subnodes_per_node = 1u<<(3*LOG2K); // 2^LOG2K (k) subnodes per dimension, 3 dimensions
	num_layers_ = num_layers;
	num_indices_ = gpu_buffer_size_ / (num_subnodes_per_node*4); // 4 bytes per sub-node

	indirection_pool_size_ = num_subnodes_per_node*4*num_indices_;
	uniformity_pool_size_ = num_subnodes_per_node*num_indices_/8;
	voxel_type_pool_size_ = num_subnodes_per_node*4*num_indices_;
	//num_indices_ = indirection_pool_size_ / (sizeof(uint32_t) * 8); // 4 byte data, 8 octants per node
	/*
	indirection_pool_ = reinterpret_cast<uint32_t*>(malloc(num_subnodes_per_node*4 * num_indices_));
	uniformity_pool_ = reinterpret_cast<char*>(calloc(1, num_subnodes_per_node*num_indices_/8));
	voxel_type_pool_ = reinterpret_cast<uint32_t*>(calloc(sizeof(uint32_t), num_subnodes_per_node*num_indices_));
	*/
	indirection_pool_ = reinterpret_cast<uint32_t*>(malloc(indirection_pool_size_));
	uniformity_pool_ = reinterpret_cast<char*>(malloc(uniformity_pool_size_));
	voxel_type_pool_ = reinterpret_cast<uint32_t*>(malloc(voxel_type_pool_size_));
	next_available_pool_index_ = 0;
	generate();
}


World::~World()
{
	free(indirection_pool_);
	free(uniformity_pool_);
	free(voxel_type_pool_);
}


World& World::operator=(const World& other)
{
	num_layers_ = other.num_layers_;
	num_indices_ = other.num_indices_;
	gpu_buffer_size_ = other.gpu_buffer_size_;
	indirection_pool_size_ = other.indirection_pool_size_;
	uniformity_pool_size_ = other.uniformity_pool_size_;
	voxel_type_pool_size_ = other.voxel_type_pool_size_;
	next_available_pool_index_ = other.next_available_pool_index_;
	indirection_pool_ = reinterpret_cast<uint32_t*>(malloc(indirection_pool_size_));
	uniformity_pool_ = reinterpret_cast<char*>(malloc(uniformity_pool_size_));
	voxel_type_pool_ = reinterpret_cast<uint32_t*>(malloc(voxel_type_pool_size_));
	//lod_pool_ = reinterpret_cast<uint32_t*>(calloc(sizeof(uint32_t), num_indices_));

	std::copy(other.indirection_pool_, other.indirection_pool_ + indirection_pool_size_, indirection_pool_);
	std::copy(other.uniformity_pool_, other.uniformity_pool_ + uniformity_pool_size_, uniformity_pool_);
	std::copy(other.voxel_type_pool_, other.voxel_type_pool_ + voxel_type_pool_size_, voxel_type_pool_);
	//std::copy(other.lod_pool_, other.lod_pool_ + (num_indices_), lod_pool_);
	return *this;
}


void World::generate()
{
	std::cout << "generating world... " << std::flush;

	// set up world as empty node
	for (unsigned int i = 0; i < (1u << (3*LOG2K)); i++)
	{
		setIndirection(0, i, 0);
		setUniformity(0, i, true);
		setVoxelType(0, i, 0);
	}
	materials_[0] = Material(0.0, 0.0, 0.0, 0.0);
	next_available_pool_index_ = 1;
	

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
	GltfHandler gltf_handler;
	Voxelizer voxelizer(gltf_handler.getMeshPtr());
	Model *model = voxelizer.createModel();
	unsigned int offset[3] = { 2048, 2048, 2048 };
	model->addToWorld(this, offset[0], offset[1], offset[2]);
	delete model;
	Material *materials = voxelizer.getMaterials();
	for (unsigned int i = 0; i < voxelizer.getNumMaterials(); i++)
	{
		materials_[i] = materials[i];
	}
	materials_[0] = Material(0.0, 0.0, 0.0, 0.0);


	std::cout << "World size: " << (next_available_pool_index_>>(18-(LOG2K*3))) << "MB" << std::endl; // *4(four bytes per 32-bit word), <<(LOG2K*3)(2^(log2k*3) children per index), >>20(B->MB)
	
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
}


uint32_t World::readIndirectionPool(uint32_t base_location, unsigned int node_index)
{
	return indirection_pool_[(base_location << (3*LOG2K)) | node_index];
}


bool World::readUniformityPool(uint32_t base_location, unsigned int node_index)
{
	uint32_t index_preunpack = (base_location << (3*LOG2K)) | node_index;
	uint32_t true_index = index_preunpack >> 3;
	uint8_t bit_location = index_preunpack & 0x7u;
	return ((uniformity_pool_[true_index] >> bit_location) & 1u) != 0;
}


uint32_t World::readVoxelTypePool(uint32_t base_location, unsigned int node_index)
{
	return voxel_type_pool_[(base_location << (3*LOG2K)) | node_index];
}


void World::setIndirection(uint32_t base_location, unsigned int node_index, uint32_t value)
{
	indirection_pool_[(base_location << (3*LOG2K)) | node_index] = value;
	return;
}


void World::setUniformity(uint32_t base_location, unsigned int node_index, bool value)
{
	uint32_t index_preunpack = (base_location << (3*LOG2K)) | node_index;
	uint32_t true_index = index_preunpack >> 3;
	uint8_t bitfield = 0x1u << (index_preunpack & 7u);
	if (value)
		uniformity_pool_[true_index] |= bitfield;
	else
		uniformity_pool_[true_index] &= ~bitfield;
	return;
}


void World::setVoxelType(uint32_t base_location, unsigned int node_index, uint32_t value)
{
	voxel_type_pool_[(base_location << (3*LOG2K)) | node_index] = value;
}


uint32_t World::mkAndReadIndirectionPool(uint32_t base_location, unsigned int node_index)
{
	uint32_t indirection_index;
	if (readUniformityPool(base_location, node_index))
	{
		// need to create children
		uint32_t voxel_type = readVoxelTypePool(base_location, node_index);
		setUniformity(base_location, node_index, false);
		setIndirection(base_location, node_index, next_available_pool_index_);
		// set up new node
		for (unsigned int i = 0; i < (1u << (3*LOG2K)); i++)
		{
			setUniformity(next_available_pool_index_, i, true);
			setVoxelType(next_available_pool_index_, i, voxel_type);
			setIndirection(next_available_pool_index_, i, 0);
		}

		next_available_pool_index_++;
	}
	return readIndirectionPool(base_location, node_index);
}


void World::setVoxel(uint32_t x, uint32_t y, uint32_t z, uint32_t voxel_type)
{
	//if (voxel_type != 0) std::cout << voxel_type << std::endl;
	if (voxel_type == 0) return;
	uint32_t indirection_pointer = 0;
	uint32_t current_node_index = 0;
	uint32_t layer = num_layers_ - 1;
	uint32_t bitfield_ander = 0xFFFFFFFFu >> (32-LOG2K);
	// starting at the top layer, descend until a uniform layer is reached
	for (; layer >= 0;)
	{
		current_node_index = 0;
		current_node_index |= ((x >> (LOG2K*layer)) & bitfield_ander);
		current_node_index |= ((y >> (LOG2K*layer)) & bitfield_ander) << LOG2K; 
		current_node_index |= ((z >> (LOG2K*layer)) & bitfield_ander) << (2*LOG2K);
		if (layer == 0) break;

		if (readUniformityPool(indirection_pointer, current_node_index))
		{
			if (readVoxelTypePool(indirection_pointer, current_node_index) == voxel_type)
			{
				return;
			}
			splitNode(indirection_pointer, current_node_index);
		}

		indirection_pointer = readIndirectionPool(indirection_pointer, current_node_index);
		layer--;
	}

	setVoxelType(indirection_pointer, current_node_index, voxel_type);
	return;
}


void World::splitNode(uint32_t base_location, unsigned int node_index)
{
	uint32_t voxel_type = readVoxelTypePool(base_location, node_index);
	setUniformity(base_location, node_index, false);
	setIndirection(base_location, node_index, next_available_pool_index_);
	// set up new node
	for (unsigned int i = 0; i < (1u << (3*LOG2K)); i++)
	{
		setUniformity(next_available_pool_index_, i, true);
		setVoxelType(next_available_pool_index_, i, voxel_type);
		setIndirection(next_available_pool_index_, i, 0);
	}

	next_available_pool_index_++;
	return;
}


} // namespace Anthrax
