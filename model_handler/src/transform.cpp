/* ---------------------------------------------------------------- *\
 * transform.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-11-12
\* ---------------------------------------------------------------- */
#include <sstream> 

#include "transform.hpp"

#include <cmath>

namespace Anthrax
{

Transform::Transform(float *translation, float *rotation, float *scale)
{
	// translation is given as a vec3
	// rotation is given as a vec4 (quaternion)
	// scale is given as a vec3
	float translation_tmp[3] = {0.0, 0.0, 0.0};
	float rotation_tmp[4] = {0.0, 0.0, 0.0, 1.0};
	float scale_tmp[3] = {1.0, 1.0, 1.0};
	if (!translation) translation = translation_tmp;
	if (!rotation) rotation = rotation_tmp;
	if (!scale) scale = scale_tmp;
	float translation_matrix[4][4] =
	{
		{1.0, 0.0, 0.0, 0.0},
		{0.0, 1.0, 0.0, 0.0},
		{0.0, 0.0, 1.0, 0.0},
		{translation[0], translation[1], translation[2], 1.0}
	};
	float qx = rotation[0];
	float qy = rotation[1];
	float qz = rotation[2];
	float qw = rotation[3];
	float n = 1.0/sqrt(qx*qx+qy*qy+qz*qz+qw*qw);
	qx *= n;
	qy *= n;
	qz *= n;
	qw *= n;
	float rotation_matrix[4][4] =
	{
		{1.0f - 2.0f*qy*qy - 2.0f*qz*qz, 2.0f*qx*qy + 2.0f*qz*qw, 2.0f*qx*qz - 2.0f*qy*qw, 0.0},
		{2.0f*qx*qy - 2.0f*qz*qw, 1.0f - 2.0f*qx*qx - 2.0f*qz*qz, 2.0f*qy*qz + 2.0f*qx*qw, 0.0},
		{2.0f*qx*qz + 2.0f*qy*qw, 2.0f*qy*qz - 2.0f*qx*qw, 1.0f - 2.0f*qx*qx - 2.0f*qy*qy, 0.0},
		{0.0, 0.0, 0.0, 1.0}
	};
	float scale_matrix[4][4] =
	{
		{scale[0], 0.0, 0.0, 0.0},
		{0.0, scale[1], 0.0, 0.0},
		{0.0, 0.0, scale[2], 0.0},
		{0.0, 0.0, 0.0, 1.0}
	};

	Transform t(translation_matrix);
	Transform r(rotation_matrix);
	Transform s(scale_matrix);

	t.print();
	r.print();
	s.print();

	copy(t*r*s);

	return;
}


Transform::Transform(float matrix[4][4])
{
	for (unsigned int i = 0; i < 4; i++)
	{
		for (unsigned int j = 0; j < 4; j++)
		{
			matrix_[i][j] = matrix[i][j];
		}
	}
	return;
}


void Transform::transformVec3(float *vec3)
{
	float vec4[4] = { vec3[0], vec3[1], vec3[2], 1.0 };
	float ret[3];
	for (unsigned int col = 0; col < 3; col++)
	{
		ret[col] = 0.0;
		for (unsigned int row = 0; row < 4; row++)
		{
			ret[col] += vec4[row]*matrix_[col][row];
		}
	}
	for (unsigned int i = 0; i < 3; i++)
	{
		vec3[i] = ret[i];
	}
	return;
}


void Transform::print()
{
	std::cout << std::endl;
	for (unsigned int row = 0; row < 4; row++)
	{
		std::cout << "| ";
		for (unsigned int col = 0; col < 4; col++)
		{
			//std::cout << std::fixed << std::setprecision(3);
			//std::cout << matrix_[col][row] << " ";
			if (matrix_[col][row] >= 0.0) std::cout << " ";
			std::cout << std::fixed << std::setprecision(3) << std::setw(5) << matrix_[col][row] << " ";
		}
		std::cout << "|" << std::endl;
	}
}


Transform operator*(const Transform &left, const Transform &right)
{
	Transform return_matrix;
	for (unsigned int row = 0; row < 4; row++)
	{
		for (unsigned int col = 0; col < 4; col++)
		{
			return_matrix.matrix_[col][row] = 0;
			for (unsigned int i = 0; i < 4; i++)
			{
				return_matrix.matrix_[col][row] += left.matrix_[i][row] * right.matrix_[col][i];
			}
		}
	}
	return return_matrix;
}

} // namespace Anthrax
