/* ---------------------------------------------------------------- *\
 * mesh.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-11-11
\* ---------------------------------------------------------------- */

#include "mesh.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Anthrax
{

Mesh::Mesh()
{
}


Mesh::~Mesh()
{
	textures_.clear();
	return;
}


/*
 * Return: the triangle ID
 */
int Mesh::addTriangle(float vertices[3][3])
{
	int initial_num_triangles = triangles_.size();
	triangles_.push_back(Triangle(vertices[0], vertices[1], vertices[2], 0));
	if (triangles_.size() == initial_num_triangles+1)
		return initial_num_triangles;
	return -1;
}


/*
 * Return: the texture ID
 */
int Mesh::addTexture(std::string image_name)
{
	int initial_num_textures = textures_.size();
	textures_.push_back(Texture(image_name));
	if (textures_.size() == initial_num_textures+1)
		return initial_num_textures;
	return -1;
}


/*
 * Return: the texture ID
 */
int Mesh::addTextureFromBuffer(unsigned char *image_data, size_t image_buffer_size)
{
	int initial_num_textures = textures_.size();
	textures_.push_back(Texture(image_data, image_buffer_size));
	if (textures_.size() == initial_num_textures+1)
		return initial_num_textures;
	return -1;
}


/* ---------------------------------------------------------------- *\
 * Texture implementation
\* ---------------------------------------------------------------- */


Mesh::Texture::Texture()
{
	image_data_ = nullptr;
	return;
}


Mesh::Texture::Texture(std::string image_name)
{
	image_data_ = stbi_load(image_name.c_str(), &width_, &height_, &num_channels_, 0);
	return;
}


Mesh::Texture::Texture(unsigned char *image_data, size_t image_buffer_size)
{
	image_data_ = stbi_load_from_memory(image_data, image_buffer_size, &width_, &height_, &num_channels_, 0);
	return;
}


Mesh::Texture::~Texture()
{
	if (image_data_)
		stbi_image_free(image_data_);
	return;
}


/* ---------------------------------------------------------------- *\
 * Triangle implementation
\* ---------------------------------------------------------------- */


Mesh::Triangle::Triangle()
{
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
			vertices_[i][j] = 0.0;
	}
	texture_id_ = -1;
	return;
}


Mesh::Triangle::Triangle(float vtx0[], float vtx1[], float vtx2[], int texture_id)
{
	for (int i = 0; i < 3; i++)
	{
		vertices_[0][i] = vtx0[i];
		vertices_[1][i] = vtx1[i];
		vertices_[2][i] = vtx2[i];
	}
	texture_id_ = texture_id;
	return;
}


void Mesh::Triangle::copy(const Mesh::Triangle &other)
{
	for (unsigned int i = 0; i < 3; i++)
	{
		for (unsigned int j = 0; j < 3; j++)
		{
			vertices_[i][j] = other.vertices_[i][j];
		}
	}
}


} // namespace Anthrax
