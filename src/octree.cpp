/* ---------------------------------------------------------------- *\
 * octree.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-02-03
\* ---------------------------------------------------------------- */
#include "octree.hpp"

#include <cstdlib>
#include <algorithm>
#include <iostream>

namespace Anthrax
{

Octree::Octree(int num_layers)
{
  num_layers_ = num_layers;
  num_indices_ = indirection_pool_size_ / (sizeof(uint32_t) * 8); // 4 byte data, 8 octants per node
  indirection_pool_ = reinterpret_cast<uint32_t*>(malloc(8 * sizeof(uint32_t) * num_indices_));
  voxel_type_pool_ = reinterpret_cast<uint16_t*>(calloc(sizeof(uint16_t), num_indices_));
  lod_pool_ = reinterpret_cast<uint32_t*>(calloc(sizeof(uint32_t), num_indices_));
}


Octree::~Octree()
{
  free(indirection_pool_);
  free(voxel_type_pool_);
  free(lod_pool_);
}


Octree& Octree::operator=(const Octree& other)
{
  num_layers_ = other.num_layers_;
  num_indices_ = other.num_indices_;
  indirection_pool_size_ = other.indirection_pool_size_;
  indirection_pool_ = reinterpret_cast<uint32_t*>(malloc(indirection_pool_size_));
  voxel_type_pool_ = reinterpret_cast<uint16_t*>(calloc(sizeof(uint16_t), num_indices_));
  lod_pool_ = reinterpret_cast<uint32_t*>(calloc(sizeof(uint32_t), num_indices_));

  std::copy(other.indirection_pool_, other.indirection_pool_ + (8 * num_indices_), indirection_pool_);
  std::copy(other.voxel_type_pool_, other.voxel_type_pool_ + (sizeof(uint16_t) * num_indices_), voxel_type_pool_);
  std::copy(other.lod_pool_, other.lod_pool_ + (sizeof(uint32_t) * num_indices_), lod_pool_);
  return *this;
}

} // namespace Anthrax
