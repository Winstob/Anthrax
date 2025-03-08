/* ---------------------------------------------------------------- *\
 * model.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-12-12
\* ---------------------------------------------------------------- */
#ifndef MODEL_HPP
#define MODEL_HPP

#include <stdlib.h>
#include <cstdint>
#include <string>
#include <mutex>

#include "octree.hpp"
#include "quaternion.hpp"
#include "device.hpp"
#include "compute_shader_manager.hpp"
#include "timer.hpp"

namespace Anthrax
{

class Model
{
public:
	Model(size_t size_x, size_t size_y, size_t size_z);
	Model() : Model(1, 1, 1) {};
	~Model();
	Model(const Model &other) { copy(other); }
	Model& operator=(const Model &other) { copy(other); return *this; }
	void copy(const Model &other);
	
	void setVoxel(int32_t x, int32_t y, int32_t z, uint16_t material_type);
	void rotate(Quaternion quat);
	void rotateOnLayer(Quaternion quat, int layer);
	//void addToWorld(World *world, unsigned int x, unsigned int y, unsigned int z);

	Octree *getOctree() { return octree_; }

private:
	Octree *original_octree_;
	size_t octree_width_;
	Octree *octree_;

	// rotation stuff
	Quaternion current_rotation_;
	size_t lowest_rotated_layer_;
	void rotateOnLayer(Quaternion quat);
	void rotateVoxel(int *x, int *y, int *z, float xangle, float yangle, float zangle);
	void unrotateVoxel(int *x, int *y, int *z, float xangle, float yangle, float zangle);

	void rotateVoxelSingleAxis(int *x, int *y, int *z, float angle, int axis, int mode);
	void rotateVoxelYaw(int *x, int *y, int *z, float angle);
	void rotateVoxelPitch(int *x, int *y, int *z, float angle);
	void rotateVoxelRoll(int *x, int *y, int *z, float angle);
	void unrotateVoxelYaw(int *x, int *y, int *z, float angle);
	void unrotateVoxelPitch(int *x, int *y, int *z, float angle);
	void unrotateVoxelRoll(int *x, int *y, int *z, float angle);

	// Rotation buffers will naturally be large, so this whole operation is made static
	void rotateGPU(Quaternion quat);
	void recreateRotationShaders();
	struct RotationStuff
	{
		std::mutex mutex;
		size_t buffer_size;
		ComputeShaderManager shader_manager;
		Buffer cpu_ssbo, gpu_ssbo, freelist_ssbo;
		Buffer rotation_angles_ubo, octree_depth_ubo;
		std::vector<Descriptor> shader_descriptors;
		VkFence fence;
		VkCommandPool command_pool;
		VkCommandBuffer command_buffer;
		bool initialized = false;;
	};
	static RotationStuff rotation_stuff_;
	void rotationStuffSetup();
	struct RotationAngles
	{
		alignas(16) glm::vec3 old_angles;
		alignas(16) glm::vec3 new_angles;
	};
	Quaternion old_rotation_;
};

} // namespace Anthrax
#endif // MODEL_HPP
