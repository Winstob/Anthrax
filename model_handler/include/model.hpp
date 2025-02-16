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
	void mainSetup(size_t size_x, size_t size_y, size_t size_z);

	Octree *original_octree_;
	size_t size_;
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
};

} // namespace Anthrax
#endif // MODEL_HPP
