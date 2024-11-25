/* ---------------------------------------------------------------- *\
 * voxelizer.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-11-11
\* ---------------------------------------------------------------- */

#include "voxelizer.hpp"

namespace Anthrax
{

Voxelizer::Voxelizer(Mesh *mesh)
{
	mesh_ = mesh;
	materials_ = reinterpret_cast<Material*>(malloc(num_materials_*sizeof(Material)));
	for (unsigned int i = 0; i < num_materials_; i++)
	{
		unsigned int red = (i >> 8) & 15;
		unsigned int green = (i >> 4) & 15;
		unsigned int blue = i & 15;
		materials_[i] = Material(static_cast<float>(red)/15.0, static_cast<float>(green)/15.0, static_cast<float>(blue)/15.0);
		//std::cout << i << ": r" << static_cast<float>(red)/7.0 << " g" << static_cast<float>(green)/7.0 << " b" << static_cast<float>(blue)/7.0 << std::endl;
	}
	return;
}


void Voxelizer::copy(const Voxelizer &other)
{
	mesh_ = other.mesh_;
	num_materials_ = other.num_materials_;
	if (materials_) free(materials_);
	materials_ = reinterpret_cast<Material*>(malloc(num_materials_*sizeof(Material)));
	memcpy(materials_, other.materials_, num_materials_*sizeof(Material));
	return;
}


Voxelizer::~Voxelizer()
{
	free(materials_);
	return;
}


void Voxelizer::addToWorld(World *world)
{
	unsigned int offset[3] = { 2048, 2048, 2048 };
	float multiplier = 40.0;
	for (unsigned int i = 0; i < mesh_->size(); i++)
	{
		Mesh::Triangle triangle = (*mesh_)[i];
		float world_locations[3][3]; // the vertex positions in the world
		for (unsigned int vertex = 0; vertex < 3; vertex++)
		{
			// manipulate world position
			world_locations[vertex][0] = triangle[vertex][0]*multiplier;
			world_locations[vertex][1] = triangle[vertex][1]*multiplier;
			world_locations[vertex][2] = triangle[vertex][2]*multiplier;
		}
		triangle.scale(multiplier);
		// find min/max for each axis
		int mins[3];
		int maxes[3];
		for (unsigned int axis = 0; axis < 3; axis++)
		{
			float min = world_locations[0][axis];
			if (world_locations[1][axis] < min) min = world_locations[1][axis];
			if (world_locations[2][axis] < min) min = world_locations[2][axis];
			float max = world_locations[0][axis];
			if (world_locations[1][axis] > max) max = world_locations[1][axis];
			if (world_locations[2][axis] > max) max = world_locations[2][axis];
			mins[axis] = static_cast<int>(min);
			maxes[axis] = static_cast<int>(max);
		}
		for (int x = mins[0]; x <= maxes[0]; x++)
		{
			for (int y = mins[1]; y <= maxes[1]; y++)
			{
				for (int z = mins[2]; z <= maxes[2]; z++)
				{
					if (intersectionCheck(world_locations, x, y, z))
					{
						float test_point[3] = { static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) };
						//projectOntoTrianglePlane(test_point, triangle);
						int material = getMaterial(triangle, test_point);
						world->setVoxel(x+offset[0], y+offset[1], z+offset[2], material);
					}
				}
			}
		}

		//world->setVoxel(world_location[0], world_location[1], world_location[2], 1);
	}
	return;
}


bool Voxelizer::intersectionCheck(float vertices[3][3], int x, int y, int z)
{
	float x_float = static_cast<float>(x);
	float y_float = static_cast<float>(y);
	float z_float = static_cast<float>(z);
	float *a = vertices[0];
	float *b = vertices[1];
	float *c = vertices[2];
	float ab[3];
	float ac[3];
	// create two vectors tangeant to the plane
	ab[0] = b[0] - a[0];
	ab[1] = b[1] - a[1];
	ab[2] = b[2] - a[2];
	ac[0] = c[0] - a[0];
	ac[1] = c[1] - a[1];
	ac[2] = c[2] - a[2];
	// find normal vector by calculating the cross-product of ab and ac
	float normal[3];
	cross(normal, ab, ac);
	// find the constant "d" for the plane equation
	float d = -1.0*(normal[0]*a[0] + normal[1]*a[1] + normal[2]*a[2]);
	if (abs(normal[0]*x + normal[1]*y + normal[2]*z + d) <= 0.5)
	{
		// this intersects the triangle's plane - check to make sure it is also within the triangle's bouds
		// project the test point onto the plane
		float test_point[3];
		float distance_to_plane = normal[0]*x_float + normal[1]*y_float + normal[2]*z_float + d;
		test_point[0] = x_float - normal[0]*distance_to_plane;
		test_point[1] = y_float - normal[1]*distance_to_plane;
		test_point[2] = z_float - normal[2]*distance_to_plane;
		// if the angle between all vectors adds up to 2*pi, the test point is inside the triangle
		float ap[3];
		ap[0] = test_point[0] - a[0];
		ap[1] = test_point[1] - a[1];
		ap[2] = test_point[2] - a[2];
		float bp[3];
		bp[0] = test_point[0] - b[0];
		bp[1] = test_point[1] - b[1];
		bp[2] = test_point[2] - b[2];
		float cp[3];
		cp[0] = test_point[0] - c[0];
		cp[1] = test_point[1] - c[1];
		cp[2] = test_point[2] - c[2];
		float angle1 = acos(dot(ap, bp) / (magnitude(ap)*magnitude(bp)));
		float angle2 = acos(dot(bp, cp) / (magnitude(bp)*magnitude(cp)));
		float angle3 = acos(dot(cp, ap) / (magnitude(cp)*magnitude(ap)));
		if (angle1 + angle2 + angle3 >= 6.28) return true;
		// failed, but try moving it just a little bit towards the center and check again.
		// TODO: find a less hacky way to do this
		// find centroid
		float centroid[3];
		for (unsigned int axis = 0; axis < 3; axis++)
			centroid[axis] = (a[axis]+b[axis]+c[axis])/3.0;
		float move_dir[3];
		move_dir[0] = centroid[0] - test_point[0];
		move_dir[1] = centroid[1] - test_point[1];
		move_dir[2] = centroid[2] - test_point[2];
		normalize(move_dir);
		float move_magnitude = 0.25;
		test_point[0] += move_dir[0]*move_magnitude;
		test_point[1] += move_dir[1]*move_magnitude;
		test_point[2] += move_dir[2]*move_magnitude;
		// now try again
		ap[0] = test_point[0] - a[0];
		ap[1] = test_point[1] - a[1];
		ap[2] = test_point[2] - a[2];
		bp[0] = test_point[0] - b[0];
		bp[1] = test_point[1] - b[1];
		bp[2] = test_point[2] - b[2];
		cp[0] = test_point[0] - c[0];
		cp[1] = test_point[1] - c[1];
		cp[2] = test_point[2] - c[2];
		angle1 = acos(dot(ap, bp) / (magnitude(ap)*magnitude(bp)));
		angle2 = acos(dot(bp, cp) / (magnitude(bp)*magnitude(cp)));
		angle3 = acos(dot(cp, ap) / (magnitude(cp)*magnitude(ap)));
		if (angle1 + angle2 + angle3 >= 6.28) return true;
	}
	return false;
}


void Voxelizer::projectOntoTrianglePlane(float *point, Mesh::Triangle triangle)
{
	float *a = triangle.vertices_[0];
	float *b = triangle.vertices_[1];
	float *c = triangle.vertices_[2];
	float ab[3];
	float ac[3];
	// create two vectors tangeant to the plane
	ab[0] = b[0] - a[0];
	ab[1] = b[1] - a[1];
	ab[2] = b[2] - a[2];
	ac[0] = c[0] - a[0];
	ac[1] = c[1] - a[1];
	ac[2] = c[2] - a[2];
	// find normal vector by calculating the cross-product of ab and ac
	float normal[3];
	cross(normal, ab, ac);
	// find the constant "d" for the plane equation
	float d = -1.0*(normal[0]*a[0] + normal[1]*a[1] + normal[2]*a[2]);

	float distance_to_plane = normal[0]*point[0] + normal[1]*point[1] + normal[2]*point[2] + d;
	point[0] = point[0] - normal[0]*distance_to_plane;
	point[1] = point[1] - normal[1]*distance_to_plane;
	point[2] = point[2] - normal[2]*distance_to_plane;
	return;
}


int Voxelizer::getMaterial(Mesh::Triangle triangle, float test_point[3])
{
	//float texcoord[2] = { 0.5, 0.5 };
	float texcoord[2];

	/*
	float vertex_distances[3];
	for (unsigned int vertex = 0; vertex < 3; vertex++)
	{
		float vert_to_point[3];
		for (unsigned int axis = 0; axis < 3; axis++)
		{
			vert_to_point[axis] = test_point[axis] - triangle.vertices_[vertex][axis];
		}
		vertex_distances[vertex] = magnitude(vert_to_point);
	}
	float td = vertex_distances[0] + vertex_distances[1];
	float r12 = vertex_distances[0]/td;
	float v12[2];
	v12[0] = lerp(triangle.texture_coords_[0][0], triangle.texture_coords_[1][0], r12);
	v12[1] = lerp(triangle.texture_coords_[0][1], triangle.texture_coords_[1][1], r12);
	float p12[3];
	p12[0] = lerp(triangle.vertices_[0][0], triangle.vertices_[0][1], r12);
	p12[1] = lerp(triangle.vertices_[1][0], triangle.vertices_[1][1], r12);
	p12[2] = lerp(triangle.vertices_[2][0], triangle.vertices_[2][1], r12);
	float p12_to_point[3];
	p12_to_point[0] = p12[0] - test_point[0];
	p12_to_point[1] = p12[1] - test_point[1];
	p12_to_point[2] = p12[2] - test_point[2];
	float d12 = magnitude(p12_to_point);
	td = d12 + vertex_distances[2];
	float r123 = d12/td;
	float v123[2];
	v123[0] = lerp(v12[0], triangle.texture_coords_[2][0], r123);
	v123[1] = lerp(v12[1], triangle.texture_coords_[2][1], r123);
	texcoord[0] = v123[0];
	texcoord[1] = v123[1];
	*/
	// barycentric coordinates
	float *p = test_point;
	float *v1 = triangle.vertices_[0];
	float *v2 = triangle.vertices_[1];
	float *v3 = triangle.vertices_[2];
	float *t1 = triangle.texture_coords_[0];
	float *t2 = triangle.texture_coords_[1];
	float *t3 = triangle.texture_coords_[2];
	float e1[3];
	e1[0] = v2[0] - v1[0];
	e1[1] = v2[1] - v1[1];
	e1[2] = v2[2] - v1[2];
	float e2[3];
	e2[0] = v3[0] - v1[0];
	e2[1] = v3[1] - v1[1];
	e2[2] = v3[2] - v1[2];
	float d[3];
	d[0] = p[0] - v1[0];
	d[1] = p[1] - v1[1];
	d[2] = p[2] - v1[2];
	float dot00 = dot(e1, e1);
	float dot01 = dot(e1, e2);
	float dot11 = dot(e2, e2);
	float dotd0 = dot(d, e1);
	float dotd1 = dot(d, e2);
	float denom = dot00 * dot11 - dot01 * dot01;
	if (denom == 0.0) denom = 0.1;
	float u = (dot11 * dotd0 - dot01 * dotd1) / denom;
	float v = (dot00 * dotd1 - dot01 * dotd0) / denom;
	float w = 1 - u - v;
	texcoord[0] = w*t1[0] + u*t2[0] + v*t3[0];
	texcoord[1] = w*t1[1] + u*t2[1] + v*t3[1];
	

	std::vector<float> color = triangle.sampleTexture(texcoord);
	unsigned int red = static_cast<unsigned int>(color[0]*15.0);
	unsigned int green = static_cast<unsigned int>(color[1]*15.0);
	unsigned int blue = static_cast<unsigned int>(color[2]*15.0);
	if (red == 0 && green == 0 && blue == 0)
	{
		red = 1;
		green = 1;
		blue = 1;
	}
	return ((red<<8) | (green<<4) | (blue));
}


float Voxelizer::lerp(float a, float b, float t)
{
	return a + t * (b - a);
}


void Voxelizer::normalize(float *vec3)
{
	float normalize_factor = sqrt(vec3[0]*vec3[0] + vec3[1]*vec3[1] + vec3[2]*vec3[2]);
	vec3[0] /= normalize_factor;
	vec3[1] /= normalize_factor;
	vec3[2] /= normalize_factor;
	return;
}


void Voxelizer::cross(float *result, float vec1[3], float vec2[3])
{
	result[0] = vec1[1]*vec2[2] - vec1[2]*vec2[1];
	result[1] = vec1[2]*vec2[0] - vec1[0]*vec2[2];
	result[2] = vec1[0]*vec2[1] - vec1[1]*vec2[0];
	normalize(result);
	return;
}


float Voxelizer::magnitude(float vec3[3])
{
	return sqrt(vec3[0]*vec3[0] + vec3[1]*vec3[1] + vec3[2]*vec3[2]);
}


float Voxelizer::dot(float vec1[3], float vec2[3])
{
	return vec1[0]*vec2[0] + vec1[1]*vec2[1] + vec1[2]*vec2[2];
}

} // namespace Anthrax
