/* ---------------------------------------------------------------- *\
 * flat_octree.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-02-03
\* ---------------------------------------------------------------- */
#ifndef FLAT_OCTREE_HPP
#define FLAT_OCTREE_HPP

#include <stdint.h>
#include <cstddef>

#define KB(x) ((size_t) (x) << 10)
#define MB(x) ((size_t) (x) << 20)
#define GB(x) ((size_t) (x) << 30)

namespace Anthrax
{

class FlatOctree
{
public:
	FlatOctree(int num_layers);
	FlatOctree() : FlatOctree(8) {};
	~FlatOctree();
	FlatOctree& operator=(const FlatOctree& other);


	int num_layers_;
	int num_indices_;
	uint32_t *indirection_pool_; // 0x0000 marks air
						 // Any other marks the index of that child
						 // Index must be multiplied by 8: 8 octants per node
	uint32_t *voxel_type_pool_; // If an element != to 0x00, it is a leaf (uniform non-air octant)
	uint32_t *lod_pool_; // Stores color data for every element in the indirection pool

private:
	size_t indirection_pool_size_ = MB(4); // The size of the indirection pool in bytes
};

} // namespace Anthrax
#endif // FLAT_OCTREE_HPP
