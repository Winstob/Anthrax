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
	
	bool isUniform() { return (layer_ == 0 || getUniformity()); }
	VoxelTypeElement getMaterialType()
	{
		if (isRoot()) return 0;
		return (*voxel_type_pool_)[(parent_->pool_index_<<3)+this_child_index_];
	}
	void setMaterialType(VoxelTypeElement material_type)
	{
		if (isRoot()) return;
		(*voxel_type_pool_)[(parent_->pool_index_<<3)+this_child_index_] = material_type;
		return;
	}
	int getLayer() { return layer_; }
	bool isRoot() { return (parent_ == nullptr); }
	
	void setVoxel(int32_t x, int32_t y, int32_t z, uint16_t material_type);
	void setVoxelAtLayer(int32_t x, int32_t y, int32_t z, uint16_t material_type, int layer);
	VoxelTypeElement getVoxel(int32_t x, int32_t y, int32_t z);
	VoxelTypeElement getVoxelAtLayer(int32_t x, int32_t y, int32_t z, int layer);
	void mergeOctree(Octree *other, int32_t x, int32_t y, int32_t z);

	enum SplitMode
	{
		SPLIT_MODE_NORMAL, // default value, probably should be used for most cases
		SPLIT_MODE_AIRFILL // useful for breadth-first (ex. rotation)
	};
	void setSplitMode(SplitMode mode) { *split_mode_ = mode; }

	IndirectionElement *getIndirectionPool() { return indirection_pool_->data(); }
	VoxelTypeElement *getVoxelTypePool() { return voxel_type_pool_->data(); }
	UniformityElement *getUniformityPool() { return uniformity_pool_->data(); }
	size_t getIndirectionPoolSize() { return indirection_pool_->size(); }
	size_t getVoxelTypePoolSize() { return voxel_type_pool_->size(); }
	size_t getUniformityPoolSize() { return uniformity_pool_->size(); }
private:
	int layer_;
	Octree *parent_;
	Octree *children_;
	int this_child_index_;
	SplitMode *split_mode_;

	void simpleMerge();
	void simpleUpdateLOD();
	VoxelTypeElement calculateMaterialTypeFromChildren();
	unsigned int prepareForDescent(int32_t *x, int32_t *y, int32_t *z);
	void split();

	Freelist *pool_freelist_;
	std::vector<IndirectionElement> *indirection_pool_;
	std::vector<VoxelTypeElement> *voxel_type_pool_;
	std::vector<UniformityElement> *uniformity_pool_;
	size_t pool_index_;
	void setUniformity(bool uniformity)
	{
		if (isRoot()) return;
		IndirectionElement tmp = (uniformity) ? 0 : pool_index_;
		(*indirection_pool_)[(parent_->pool_index_<<3)+this_child_index_] = tmp;
		return;
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
		if (isRoot()) return false;
		return ((*indirection_pool_)[(parent_->pool_index_<<3)+this_child_index_]
				== 0);
		
		unsigned int shifter = Anthrax::log2(sizeof(UniformityElement)*8);
		size_t index = pool_index_ >> shifter;
		unsigned int subindex = ~(index << shifter) & pool_index_;
		UniformityElement bitfield = static_cast<UniformityElement>(1u)
				<< (8*sizeof(UniformityElement) - subindex - 1);
		return (*uniformity_pool_)[index] & bitfield;
	}
	void setIndirection(int child_index, IndirectionElement pool_location)
	{
		if (isRoot()) return;
		(*indirection_pool_)[(parent_->pool_index_<<3)+child_index] = pool_location;
		return;
	}

	void mergeIntoOctree(Octree *other, int32_t x, int32_t y, int32_t z);
};

} // namespace Anthrax
#endif // OCTREE_HPP
