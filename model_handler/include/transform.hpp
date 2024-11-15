/* ---------------------------------------------------------------- *\
 * transform.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-11-12
\* ---------------------------------------------------------------- */

#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <vector>
#include <map>

#include "tools.hpp"

namespace Anthrax
{

class World;

class Transform
{
// Stored as a column-major 4x4 matrix
public:
	Transform(float *translation, float *rotation, float *scale);
	Transform(float matrix[4][4]);
	Transform() {}
	Transform (const Transform &other) { copy(other); }
	Transform &operator=(const Transform &other) { copy(other); return *this; }
	friend Transform operator*(const Transform &left, const Transform &right);

	Transform transpose()
	{
		Transform new_matrix;
		for (unsigned int i = 0; i < 4; i++)
		{
			for (unsigned int j = 0; j < 4; j++)
			{
				new_matrix.matrix_[i][j] = matrix_[j][i];
			}
		}
		return new_matrix;
	}
	void copy(const Transform &other)
	{
		for (unsigned int i = 0; i < 4; i++)
		{
			for (unsigned int j = 0; j < 4; j++)
			{
				matrix_[i][j] = other.matrix_[i][j];
			}
		}
	}
	void print();
private:
	float matrix_[4][4] = { {1.0, 0.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 0.0}, {0.0, 0.0, 1.0, 0.0}, {0.0, 0.0, 0.0, 1.0} };
};

} // namespace Anthrax

#endif // TRANSFORM_HPP
