/* ---------------------------------------------------------------- *\
 * mesh.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-11-11
\* ---------------------------------------------------------------- */

#ifndef MESH_HPP
#define MESH_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>

#include "tools.hpp"

#include "material.hpp"

namespace Anthrax
{

class Mesh
{
public:
	Mesh();
	~Mesh();
	class Triangle
	{
	public:
		Triangle();
		Triangle(float vtx0[], float vtx1[], float vtx2[], int texture_id);
		~Triangle() {};
		Triangle(const Triangle &other) { copy(other); }
		Triangle &operator=(const Triangle &other) { copy(other); return *this; }
		void copy(const Triangle &other);
		float *operator[](size_t index);
	private:
		float vertices_[3][3];
		int texture_id_;
		// TODO: UV coords?
	};
	int addTriangle(float vertices[3][3]);
	int addTexture(std::string image_name);
	int addTextureFromBuffer(unsigned char *image_data, size_t image_buffer_size);
	Triangle operator[](size_t index);
	size_t size() { return triangles_.size(); }
private:
	class Texture
	{
	public:
		Texture();
		~Texture();
		Texture(std::string image_name);
		Texture(unsigned char *image_data, size_t image_buffer_size);
	private:
		unsigned char *image_data_;
		int width_, height_;
		int num_channels_;
	};
	std::vector<Texture> textures_;
	std::vector<Triangle> triangles_;
};

} // namespace Anthrax

#endif // MESH_HPP
