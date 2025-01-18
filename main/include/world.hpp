/* ---------------------------------------------------------------- *\
 * world.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-08-08
\* ---------------------------------------------------------------- */

#ifndef WORLD_HPP
#define WORLD_HPP

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include "tools.hpp"
#include "material.hpp"
#include "device.hpp"
#include "octree.hpp"
#include "model.hpp"

#define LOG2K 1

namespace Anthrax
{

class World
{
public:
	World(int num_layers, Device gpu_device);
	World(int num_layers);
	World() { World(1); }
	~World();
	World& operator=(const World &other);

	IndirectionElement* getIndirectionPool() { return octree_->getIndirectionPool(); }
	VoxelTypeElement* getVoxelTypePool() { return octree_->getVoxelTypePool(); }
	int getNumLayers() { return octree_->getLayer(); }
	Material* getMaterialsPtr() { return materials_; }
	size_t getNumMaterials() { return num_materials_; }
	size_t getIndirectionPoolSize()
	{
		return octree_->getIndirectionPoolSize()*sizeof(IndirectionElement);
	}
	size_t getVoxelTypePoolSize()
	{
		return octree_->getVoxelTypePoolSize()*sizeof(VoxelTypeElement);
	}
	// These assume the indirection pool will always be the largest
	size_t getMaxIndirectionPoolSize() { return max_gpu_buffer_size_; }
	size_t getMaxVoxelTypePoolSize()
	{
		return max_gpu_buffer_size_ / (8*sizeof(IndirectionElement))
				* 8*sizeof(VoxelTypeElement);
	}

	void generate();
	void setVoxel(int32_t x, int32_t y, int32_t z, int32_t voxel_type)
	{
		octree_->setVoxel(x, y, z, voxel_type);
	}
	void clear() { octree_->clear(); }
	void addModel(Model *model, int32_t x_offset, int32_t y_offset,
			int32_t z_offset);

private:
	void mainSetup(int num_layers);

	Octree *octree_;

	size_t num_materials_ = 4096;
	Material materials_[4096];

	void generateSerpinskiPyramidNode(unsigned int index);
	void generateSingleSerpinskiPyramidNode(unsigned int node_index, int num_layers, int layer, unsigned int x, unsigned int y, unsigned int z, bool is_air);

	size_t max_gpu_buffer_size_ = MB(512); // The maximum size of a buffer in the GPU

	Device device_;

};

} // namespace Anthrax

#endif // WORLD_HPP
