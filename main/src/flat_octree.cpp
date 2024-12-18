/* ---------------------------------------------------------------- *\
 * flat_octree.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-02-03
\* ---------------------------------------------------------------- */
#include "flat_octree.hpp"

#include <cstdlib>
#include <algorithm>
#include <iostream>

namespace Anthrax
{

FlatOctree::FlatOctree(int num_layers)
{
	num_layers_ = num_layers;
	num_indices_ = indirection_pool_size_ / (sizeof(uint32_t) * 8); // 4 byte data, 8 octants per node
	indirection_pool_ = reinterpret_cast<uint32_t*>(malloc(8 * sizeof(uint32_t) * num_indices_));
	voxel_type_pool_ = reinterpret_cast<uint32_t*>(calloc(sizeof(uint32_t), num_indices_));
	lod_pool_ = reinterpret_cast<uint32_t*>(calloc(sizeof(uint32_t), num_indices_));
}


FlatOctree::~FlatOctree()
{
	free(indirection_pool_);
	free(voxel_type_pool_);
	free(lod_pool_);
}


FlatOctree& FlatOctree::operator=(const FlatOctree& other)
{
	num_layers_ = other.num_layers_;
	num_indices_ = other.num_indices_;
	indirection_pool_size_ = other.indirection_pool_size_;
	indirection_pool_ = reinterpret_cast<uint32_t*>(malloc(8 * sizeof(uint32_t) * num_indices_));
	voxel_type_pool_ = reinterpret_cast<uint32_t*>(calloc(sizeof(uint32_t), num_indices_));
	lod_pool_ = reinterpret_cast<uint32_t*>(calloc(sizeof(uint32_t), num_indices_));

	std::copy(other.indirection_pool_, other.indirection_pool_ + (8 * num_indices_), indirection_pool_);
	std::copy(other.voxel_type_pool_, other.voxel_type_pool_ + (num_indices_), voxel_type_pool_);
	std::copy(other.lod_pool_, other.lod_pool_ + (num_indices_), lod_pool_);
	return *this;
}

} // namespace Anthrax
