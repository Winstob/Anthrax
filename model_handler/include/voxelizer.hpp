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
#include "model.hpp"
#include "device.hpp"

namespace Anthrax
{

class Voxelizer
{
public:
	Voxelizer(Mesh *mesh, Device gpu_device);
	Voxelizer(Mesh *mesh);
	Voxelizer() : Voxelizer(nullptr) {}
	Voxelizer(const Voxelizer &other) { copy(other); }
	Voxelizer &operator=(const Voxelizer &other) { copy(other); return *this; }
	void copy(const Voxelizer &other);
	~Voxelizer();
	Model *createModel();
	unsigned int getNumMaterials() { return num_materials_; }
	Material *getMaterials() { return materials_; }
private:
	void mainSetup(Mesh *mesh);

	bool intersectionCheck(float vertices[3][3], int x, int y, int z);
	void projectOntoTrianglePlane(float *point, Mesh::Triangle triangle);
	int getMaterial(Mesh::Triangle triangle, float test_point[3]);
	float lerp(float a, float b, float t);
	void normalize(float *vec3);
	void cross(float *result, float vec1[3], float vec2[3]);
	float magnitude(float vec3[3]);
	float dot(float vec1[3], float vec2[3]);
	Mesh *mesh_;
	Material *materials_;
	unsigned int num_materials_ = 4096;

	Device device_;
	bool has_gpu_device_;
};

} // namespace Anthrax

#endif // VOXELIZER_HPP
