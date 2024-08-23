/* ---------------------------------------------------------------- *\
 * world.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-08-08
\* ---------------------------------------------------------------- */
#include "world.hpp"

#include <cstdlib>
#include <algorithm>
#include <iostream>

namespace Anthrax
{

World::World(int num_layers)
{
	if (num_layers < 1)
	{
		throw std::runtime_error("World cannot have zero levels!");
	}
	num_layers_ = num_layers;
	//num_indices_ = indirection_pool_size_ / (sizeof(uint32_t) * 8); // 4 byte data, 8 octants per node
	num_indices_ = indirection_pool_size_ / (512*4); // 512 sub-nodes per node, 4 bytes per sub-node
	indirection_pool_ = reinterpret_cast<uint32_t*>(malloc(512*4 * num_indices_));
	uniformity_pool_ = reinterpret_cast<char*>(calloc(1, num_indices_/8));
	voxel_type_pool_ = reinterpret_cast<uint32_t*>(calloc(sizeof(uint32_t), num_indices_));
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
	indirection_pool_size_ = other.indirection_pool_size_;
	indirection_pool_ = reinterpret_cast<uint32_t*>(malloc(8 * sizeof(uint32_t) * num_indices_));
	voxel_type_pool_ = reinterpret_cast<uint32_t*>(calloc(sizeof(uint32_t), num_indices_));
	//lod_pool_ = reinterpret_cast<uint32_t*>(calloc(sizeof(uint32_t), num_indices_));

	std::copy(other.indirection_pool_, other.indirection_pool_ + (8 * num_indices_), indirection_pool_);
	std::copy(other.voxel_type_pool_, other.voxel_type_pool_ + (num_indices_), voxel_type_pool_);
	//std::copy(other.lod_pool_, other.lod_pool_ + (num_indices_), lod_pool_);
	return *this;
}


void World::generate()
{
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
}


} // namespace Anthrax
