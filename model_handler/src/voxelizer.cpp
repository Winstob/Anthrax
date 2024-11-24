/* ---------------------------------------------------------------- *\
 * voxelizer.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-11-11
\* ---------------------------------------------------------------- */

#include "voxelizer.hpp"

namespace Anthrax
{

Voxelizer::Voxelizer()
{
	return;
}


void Voxelizer::copy(const Voxelizer &other)
{
	mesh_ = other.mesh_;
	return;
}


Voxelizer::~Voxelizer()
{
	return;
}


void Voxelizer::addToWorld(World *world)
{
	unsigned int offset[3] = { 2048, 2048, 2048 };
	float multiplier = 1.0;
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

			triangle.scale(multiplier);
		}
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
	if (abs(normal[0]*x + normal[1]*y + normal[2]*z + d) < 0.5)
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
		float *vecs[3] = { ap, bp, cp };
		for (unsigned int vector = 0; vector < 1; vector++)
		{
			float *this_vec = vecs[vector];
			float *other_vec1 = vecs[(vector+1)%3];
			float *other_vec2 = vecs[(vector+2)%3];
			float angle1 = acos(dot(this_vec, other_vec1) / (magnitude(this_vec)*magnitude(other_vec1)));
			float angle2 = acos(dot(this_vec, other_vec2) / (magnitude(this_vec)*magnitude(other_vec2)));
			float angle3 = acos(dot(other_vec1, other_vec2) / (magnitude(other_vec1)*magnitude(other_vec2)));
			if (angle1 + angle2 + angle3 >= 6.28) return true;
		}
	}
	return false;
}


int Voxelizer::getMaterial(Mesh::Triangle triangle, float test_point[3])
{
	return 1;
}


void Voxelizer::cross(float *result, float vec1[3], float vec2[3])
{
	result[0] = vec1[1]*vec2[2] - vec1[2]*vec2[1];
	result[1] = vec1[2]*vec2[0] - vec1[0]*vec2[2];
	result[2] = vec1[0]*vec2[1] - vec1[1]*vec2[0];
	// normalize the result vector
	float normalize_factor = sqrt(result[0]*result[0] + result[1]*result[1] + result[2]*result[2]);
	result[0] /= normalize_factor;
	result[1] /= normalize_factor;
	result[2] /= normalize_factor;
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
