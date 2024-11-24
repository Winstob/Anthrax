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
	triangles_.push_back(Triangle(vertices[0], vertices[1], vertices[2]));
	if (triangles_.size() == initial_num_triangles+1)
		return initial_num_triangles;
	return -1;
}


/*
 * Return: the triangle ID
 */
int Mesh::addTriangle(float vertices[3][3], int texture_id, float tex_coords[3][2])
{
	int initial_num_triangles = triangles_.size();
	triangles_.push_back(Triangle(this, vertices, texture_id, tex_coords));
	if (triangles_.size() == initial_num_triangles+1)
		return initial_num_triangles;
	return -1;
}


/*
 * Return: the image ID
 */
int Mesh::addImage(std::string image_name)
{
	int initial_num_images = images_.size();
	images_.push_back(Image(image_name));
	if (images_.size() == initial_num_images+1)
		return initial_num_images;
	return -1;
}


/*
 * Return: the image ID
 */
int Mesh::addImageFromBuffer(unsigned char *image_data, size_t image_buffer_size)
{
	int initial_num_images = images_.size();
	images_.push_back(Image(image_data, image_buffer_size));
	if (images_.size() == initial_num_images+1)
		return initial_num_images;
	return -1;
}


int Mesh::addTexture(int image_id)
{
	int initial_num_textures = textures_.size();
	textures_.push_back(Texture(this, image_id));
	if (textures_.size() == initial_num_textures+1)
		return initial_num_textures;
	return -1;
}


/* ---------------------------------------------------------------- *\
 * Image implementation
\* ---------------------------------------------------------------- */


Mesh::Image::Image()
{
	image_data_ = nullptr;
	return;
}


Mesh::Image::Image(std::string image_name)
{
	image_data_ = stbi_load(image_name.c_str(), &width_, &height_, &num_channels_, 0);
	if (!image_data_) std::cout << "POOP" << std::endl;
	return;
}


Mesh::Image::Image(unsigned char *image_data, size_t image_buffer_size)
{
	image_data_ = stbi_load_from_memory(image_data, image_buffer_size, &width_, &height_, &num_channels_, 0);
	return;
}


Mesh::Image::~Image()
{
	if (image_data_)
		stbi_image_free(image_data_);
	return;
}


void Mesh::Image::copy(const Image &other)
{
	size_t image_size = other.width_ * other.height_ * other.num_channels_;
	image_data_ = reinterpret_cast<unsigned char*>(malloc(image_size));
	memcpy(image_data_, other.image_data_, image_size);
	width_ = other.width_;
	height_ = other.height_;
	num_channels_ = other.num_channels_;
	return;
}


Mesh::Triangle Mesh::operator[](size_t index)
{
	return triangles_[index];
}

/* ---------------------------------------------------------------- *\
 * Texture implementation
\* ---------------------------------------------------------------- */

Mesh::Texture::Texture(Mesh *parent, int image_id)
{
	parent_ = parent;
	image_ = &(parent->images_[image_id]);
	return;
}


Mesh::Texture::~Texture()
{
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
	texture_ = nullptr;
	return;
}


Mesh::Triangle::Triangle(float vtx0[], float vtx1[], float vtx2[])
{
	for (int i = 0; i < 3; i++)
	{
		vertices_[0][i] = vtx0[i];
		vertices_[1][i] = vtx1[i];
		vertices_[2][i] = vtx2[i];
	}
	texture_ = nullptr;
	return;
}


Mesh::Triangle::Triangle(Mesh *parent, float vertices[3][3], int texture_id, float tex_coords[3][2])
{
	texture_ = &(parent->textures_[texture_id]);
	for (unsigned int i = 0; i < 3; i++)
	{
		for (unsigned int j = 0; j < 3; j++)
			vertices_[i][j] = vertices[i][j];
		for (unsigned int j = 0; j < 2; j++)
			texture_coords_[i][j] = tex_coords[i][j];
	}
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


float *Mesh::Triangle::operator[](size_t index)
{
	if (index > 2)
	{
		throw std::runtime_error("Triangle operator[] index out of bounds!");
	}
	return vertices_[index];
}


void Mesh::Triangle::scale(float scale_factor)
{
	for (unsigned int i = 0; i < 3; i++)
	{
		vertices_[i][0] *= scale_factor;
		vertices_[i][1] *= scale_factor;
		vertices_[i][2] *= scale_factor;
	}
	return;
}


} // namespace Anthrax
