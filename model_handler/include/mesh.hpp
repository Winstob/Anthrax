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
	class Texture;
public:
	Mesh();
	~Mesh();
	class Triangle
	{
	public:
		Triangle();
		Triangle(float vtx0[], float vtx1[], float vtx2[]);
		Triangle(Mesh *parent, float vertices[3][3], int texture_id, float tex_coords[3][2]);
		~Triangle() {};
		Triangle(const Triangle &other) { copy(other); }
		Triangle &operator=(const Triangle &other) { copy(other); return *this; }
		void copy(const Triangle &other);
		float *operator[](size_t index);
		void scale(float scale_factor);
		std::vector<float> sampleTexture(float texcoord[2]);
		friend class Voxelizer;
	private:
		float vertices_[3][3];
		Texture *texture_;
		float texture_coords_[3][2];
		// TODO: UV coords?
	};
	int addTriangle(float vertices[3][3]);
	int addTriangle(float vertices[3][3], int texture_id, float tex_coords[3][2]);
	int addImage(std::string image_name);
	int addImageFromBuffer(unsigned char *image_data, size_t image_buffer_size);
	int addSampler(int wrap_s, int wrap_t);
	int addTexture(int image_id);
	int addTexture(int image_id, int sampler_id);
	Triangle operator[](size_t index);
	size_t size() { return triangles_.size(); }
	float *getMins() { return mins_; };
	float *getMaxes() { return maxes_; };
	class Sampler
	{
	public:
		Sampler() {}
		Sampler(int wrap_s, int wrap_t) : wrap_s_(wrap_s), wrap_t_(wrap_t) {}
		int getWrapS() { return wrap_s_; }
		int getWrapT() { return wrap_t_; }
		enum WrapMode
		{
			CLAMP_TO_EDGE,
			REPEAT,
			MIRRORED_REPEAT
		};
	private:
		int wrap_s_;
		int wrap_t_;
	};
private:
	class Image
	{
	public:
		Image();
		~Image();
		Image(std::string image_name);
		Image(unsigned char *image_data, size_t image_buffer_size);
		void copy(const Image &other);
		Image(const Image &other) { copy(other); }
		Image &operator=(const Image &other) { copy(other); return *this; }
		std::vector<int> size() { std::vector<int> ret = { width_, height_ }; return ret; }
		std::vector<float> getPixel(int x, int y);
	private:
		unsigned char *image_data_;
		int width_, height_;
		int num_channels_;
	};
	class Texture
	{
	public:
		Texture(Mesh *parent, int image_id, int sampler_id);
		Texture(Mesh *parent, int image_id);
		~Texture();
		std::vector<float> sample(float texcoord[2]);
	private:
		Mesh *parent_;
		Image *image_;
		Sampler *sampler_;
	};
	std::vector<Image> images_;
	std::vector<Triangle> triangles_;
	std::vector<Sampler> samplers_;
	std::vector<Texture> textures_;
	float mins_[3] = { 0.0, 0.0, 0.0 };
	float maxes_[3] = { 0.0, 0.0, 0.0 };
};

} // namespace Anthrax

#endif // MESH_HPP
