/* ---------------------------------------------------------------- *\
 * octree.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-12-12
 *
 * This represents an unsigned octree, where the origin (0, 0, 0)
 * is located at the corner of the octree.
\* ---------------------------------------------------------------- */
#ifndef OCTREE_HPP
#define OCTREE_HPP

#include <stdlib.h>
#include <cstdint>
#include <list>

#include "freelist.hpp"
#include "quaternion.hpp"

namespace Anthrax
{

typedef uint32_t IndirectionElement;
typedef uint32_t VoxelTypeElement;

class Octree
{
public:
	Octree(int layer);
	Octree() : Octree(1) {};
	~Octree();
	Octree(const Octree &other) { copy(other); }
	Octree& operator=(const Octree &other) { copy(other); return *this; }
	void copy(const Octree &other);
	void clear();
	
	int getLayer() { return layer_; }

	void setVoxel(uint32_t x, uint32_t y, uint32_t z, VoxelTypeElement voxel_type);
	void setVoxelAtLayer(uint32_t x, uint32_t y, uint32_t z,
			VoxelTypeElement material_type, int layer);
	VoxelTypeElement getVoxel(uint32_t x, uint32_t y, uint32_t z);
	VoxelTypeElement getVoxelAtLayer(uint32_t x, uint32_t y, uint32_t z, int layer);
	void mergeOctree(Octree *other, uint32_t x, uint32_t y, uint32_t z);

	enum class SplitMode
	{
		NORMAL, // default value, probably should be used for most cases
		AIRFILL // useful for breadth-first operations (ex. rotation)
	};
	void setSplitMode(SplitMode mode) { split_mode_ = mode; }

	struct OctreeNode
	{
		alignas(sizeof(IndirectionElement)) IndirectionElement indirection;
		alignas(sizeof(VoxelTypeElement)) VoxelTypeElement voxel_type;
	};
	OctreeNode *data() { return octree_pool_->data(); }
	size_t size() { return octree_pool_->size(); }
	OctreeNode *getOctreePool() { return octree_pool_->data(); }
	size_t getOctreePoolSize() { return octree_pool_->size(); }

	static void convertToUnsignedLoc(int layer,
			int32_t x, int32_t y, int32_t z,
			uint32_t *ux, uint32_t *uy, uint32_t *uz);

	friend class Model;

	class Accessor
	{
	public:
		//Accessor() : {}
		Accessor(Octree *owner) : owner_(owner) {};
		~Accessor();
		void destroy();
		void descend(int child);
		void ascend();
		friend class Octree;
	private:
		Octree *owner_ = nullptr;
		std::list<Accessor>::iterator self_iterator_;
		std::vector<IndirectionElement> indirection_stack_;
		std::vector<int> childnum_stack_;
		int depth_ = 0;
		bool is_being_destroyed_ = false;
	};
	friend class Accessor;
	Accessor *createAccessor();
private:
	std::vector<OctreeNode> *octree_pool_;
	Freelist *pool_freelist_;
	int layer_;
	SplitMode split_mode_;

	void simpleMerge();
	void simpleUpdateLOD();
	VoxelTypeElement calculateMaterialTypeFromChildren();

	void mergeIntoOctreeRecursive(Octree *other, IndirectionElement indirection, int layer, uint32_t x, uint32_t y, uint32_t z);
	void mergeIntoOctree(Octree *other, uint32_t x, uint32_t y, uint32_t z);
	void setVoxelTypeWithinBounds(VoxelTypeElement voxel_type,
		uint32_t x_min, uint32_t y_min, uint32_t z_min,
		uint32_t x_max, uint32_t y_max, uint32_t z_max);
	uint32_t roundUpToInterval(uint32_t val, uint32_t interval);

	std::list<Accessor> accessors_;
};

} // namespace Anthrax
#endif // OCTREE_HPP
