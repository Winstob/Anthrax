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

#include "freelist.hpp"
#include "quaternion.hpp"
#include "world.hpp"

namespace Anthrax
{

typedef uint32_t IndirectionElement;
typedef uint32_t VoxelTypeElement;
typedef uint8_t UniformityElement;

class Octree
{
public:
	Octree(Octree *parent, int layer, int this_child_index);
	Octree(Octree *parent, int layer) : Octree(parent, layer, 0) {};
	Octree(int layer) : Octree(nullptr, layer) {};
	Octree() : Octree(nullptr, 0) {};
	~Octree();
	Octree(const Octree &other) { copy(other); }
	Octree& operator=(const Octree &other) { copy(other); return *this; }
	void copy(const Octree &other);
	void clear();
	
	//bool isUniform() { return (layer_ == 0) || (children_ == nullptr); }
	bool isUniform() { return getUniformity(); }
	VoxelTypeElement getMaterialType()
	{
		return (*voxel_type_pool_)[pool_index_];
	}
	void setMaterialType(VoxelTypeElement material_type)
	{
		(*voxel_type_pool_)[pool_index_] = material_type;
		return;
	}
	int getLayer() { return layer_; }
	//bool isRoot() { return (pool_index_ == 0); }
	bool isRoot() { return (!parent_); }
	
	void setVoxel(int32_t x, int32_t y, int32_t z, uint16_t material_type);
	void setVoxelAtLayer(int32_t x, int32_t y, int32_t z, uint16_t material_type, int layer);
	uint16_t getVoxel(int32_t x, int32_t y, int32_t z);
	uint16_t getVoxelAtLayer(int32_t x, int32_t y, int32_t z, int layer);
	void addToWorld(World *world, unsigned int x, unsigned int y, unsigned int z);

	enum SplitMode
	{
		SPLIT_MODE_NORMAL, // default value, probably should be used for most cases
		SPLIT_MODE_AIRFILL // useful for breadth-first (ex. rotation)
	};
	void setSplitMode(SplitMode mode) { *split_mode_ = mode; }

private:
	int layer_;
	Octree *parent_;
	Octree *children_;
	int this_child_index_;
	SplitMode *split_mode_;

	void simpleMerge();
	void simpleUpdateLOD();
	uint16_t calculateMaterialTypeFromChildren();
	unsigned int prepareForDescent(int32_t *x, int32_t *y, int32_t *z);
	void split();

	Freelist *pool_freelist_;
	std::vector<IndirectionElement> *indirection_pool_;
	std::vector<VoxelTypeElement> *voxel_type_pool_;
	std::vector<UniformityElement> *uniformity_pool_;
	size_t pool_index_;
	void setUniformity(bool uniformity)
	{
		unsigned int shifter = Anthrax::log2(sizeof(UniformityElement)*8);
		size_t index = pool_index_ >> shifter;
		unsigned int subindex = ~(index << shifter) & pool_index_;
		UniformityElement bitfield = static_cast<UniformityElement>(1u)
				<< (8*sizeof(UniformityElement) - subindex - 1);
		if (uniformity)
		{
			(*uniformity_pool_)[index] |= bitfield;
		}
		else
		{
			(*uniformity_pool_)[index] &= ~bitfield;
		}
		return;
	}
	bool getUniformity()
	{
		unsigned int shifter = Anthrax::log2(sizeof(UniformityElement)*8);
		size_t index = pool_index_ >> shifter;
		unsigned int subindex = ~(index << shifter) & pool_index_;
		UniformityElement bitfield = static_cast<UniformityElement>(1u)
				<< (8*sizeof(UniformityElement) - subindex - 1);
		return (*uniformity_pool_)[index] & bitfield;
	}
	void setIndirection(int child_index, IndirectionElement pool_location)
	{
		(*indirection_pool_)[pool_index_>>3] = pool_location;
		return;
	}
};

} // namespace Anthrax
#endif // OCTREE_HPP
