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

	void generate();

private:
	uint32_t *indirection_pool_;
	uint32_t *voxel_type_pool_;
	char *uniformity_pool_; // indexed by 64 bytes (512 bits)
	int num_layers_;
	int num_indices_;

	size_t indirection_pool_size_ = MB(4); // The size of the indirection pool in bytes

};

} // namespace Anthrax

#endif // WORLD_HPP