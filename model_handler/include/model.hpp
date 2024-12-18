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
#include "world.hpp"

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
	void addToWorld(World *world, unsigned int x, unsigned int y, unsigned int z);

private:
	Octree *octree_;
};

} // namespace Anthrax
#endif // MODEL_HPP
