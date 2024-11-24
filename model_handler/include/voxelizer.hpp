/* ---------------------------------------------------------------- *\
 * voxelizer.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-11-11
\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\
 * Voxelizer: A tool that takes in a standardized mesh and creates
 * a voxelized version of it.
 * Usage: feed in textures, then feed in triangles.
\* ---------------------------------------------------------------- */

#ifndef VOXELIZER_HPP
#define VOXELIZER_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>

#include "tools.hpp"

#include "material.hpp"
#include "mesh.hpp"
#include "world.hpp"

namespace Anthrax
{

class Voxelizer
{
public:
	Voxelizer();
	Voxelizer(Mesh *mesh) : mesh_(mesh) {}
	Voxelizer(const Voxelizer &other) { copy(other); }
	Voxelizer &operator=(const Voxelizer &other) { copy(other); return *this; }
	void copy(const Voxelizer &other);
	~Voxelizer();
	void addToWorld(World *world);
private:
	bool intersectionCheck(float vertices[3][3], int x, int y, int z);
	int getMaterial(Mesh::Triangle triangle, float test_point[3]);
	void cross(float *result, float vec1[3], float vec2[3]);
	float magnitude(float vec3[3]);
	float dot(float vec1[3], float vec2[3]);
	Mesh *mesh_;
};

} // namespace Anthrax

#endif // VOXELIZER_HPP
