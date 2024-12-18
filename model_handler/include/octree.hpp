/* ---------------------------------------------------------------- *\
 * octree.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-12-12
 *
 * The origin (0, 0, 0) is located at the center of the octree.
\* ---------------------------------------------------------------- */
#ifndef OCTREE_HPP
#define OCTREE_HPP

#include <stdlib.h>
#include <cstdint>

#include "world.hpp"

namespace Anthrax
{

class Octree
{
public:
	Octree(Octree *parent, int layer);
	Octree(int layer) : Octree(nullptr, layer) {};
	Octree() : Octree(nullptr, 0) {};
	~Octree();
	Octree(const Octree &other) { copy(other); }
	Octree& operator=(const Octree &other) { copy(other); return *this; }
	void copy(const Octree &other);
	
	bool isUniform() { return (layer_ == 0) || (children_ == nullptr); }
	uint16_t getMaterialType() { return material_type_; }
	void setMaterialType(uint16_t material_type) { material_type_ = material_type; }
	void setParentWorldIndirectionPoolIndex(uint32_t idx) { parent_world_indirection_pool_index_ = idx; }
	void setChildIndex(unsigned int idx) { this_child_index_ = idx; }
	
	void setVoxel(int32_t x, int32_t y, int32_t z, uint16_t material_type);
	void addToWorld(World *world, unsigned int x, unsigned int y, unsigned int z);

private:
	int layer_;
	Octree *parent_;
	Octree *children_;
	uint16_t material_type_;

	void simpleMerge();
	unsigned int prepareForDescent(int32_t *x, int32_t *y, int32_t *z);
	
	// used for manipulating the World object
	uint32_t parent_world_indirection_pool_index_;
	unsigned int this_child_index_;
};

} // namespace Anthrax
#endif // OCTREE_HPP
