/* ---------------------------------------------------------------- *\
 * world.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-08-08
 *
 * A class to manage a 512-tree (kd-tree where k=8,d=3).
\* ---------------------------------------------------------------- */

#ifndef WORLD_HPP
#define WORLD_HPP

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include "tools.hpp"

#include "vox_handler.hpp"
#include "gltf_handler.hpp"

#define LOG2K 1

namespace Anthrax
{

class World
{
public:
	World(int num_layers);
	World() { World(1); }
	~World();
	World& operator=(const World &other);

	uint32_t* getIndirectionPool() { return indirection_pool_; }
	char* getUniformityPool() { return uniformity_pool_; }
	uint32_t* getVoxelTypePool() { return voxel_type_pool_; }
	int getNumLayers() { return num_layers_; }
	int getNumIndices() { return num_indices_; }
	Material* getMaterialsPtr() { return materials_; }
	size_t getNumMaterials() { return num_materials_; }
	int getIndirectionPoolSize() { return indirection_pool_size_; }
	int getUniformityPoolSize() { return uniformity_pool_size_; }
	int getVoxelTypePoolSize() { return voxel_type_pool_size_; }

	uint32_t readIndirectionPool(uint32_t base_location, unsigned int node_index);
	bool readUniformityPool(uint32_t base_location, unsigned int node_index);
	uint32_t readVoxelTypePool(uint32_t base_location, unsigned int node_index);
	void setIndirection(uint32_t base_location, unsigned int node_index, uint32_t value);
	void setUniformity(uint32_t base_location, unsigned int node_index, bool value);
	void setVoxelType(uint32_t base_location, unsigned int node_index, uint32_t value);

	void generate();
	void setVoxel(uint32_t x, uint32_t y, uint32_t z, uint32_t voxel_type);

private:
	size_t num_materials_ = 4096;
	Material materials_[4096];
	uint32_t *indirection_pool_;
	uint32_t *voxel_type_pool_;
	char *uniformity_pool_; // indexed by 64 bytes (512 bits)
	int num_layers_;
	int num_indices_;
	size_t indirection_pool_size_;
	size_t uniformity_pool_size_;
	size_t voxel_type_pool_size_;
	uint32_t next_available_pool_index_;

	void generateSerpinskiPyramidNode(unsigned int index);
	void generateSingleSerpinskiPyramidNode(unsigned int node_index, int num_layers, int layer, unsigned int x, unsigned int y, unsigned int z, bool is_air);
	void splitNode(uint32_t base_location, unsigned int node_index);

	size_t gpu_buffer_size_ = MB(512); // The maximum size of a buffer in the GPU


};

} // namespace Anthrax

#endif // WORLD_HPP
