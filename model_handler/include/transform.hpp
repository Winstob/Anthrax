/* ---------------------------------------------------------------- *\
 * transform.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-11-12
\* ---------------------------------------------------------------- */

#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <fstream>
#include <iostream>
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
	class Model;
	class Dict;
	class TransformNode;
	class GroupNode;
	class ShapeNode;
	class SceneNode;
public:
	Transform() {};
	~Transform();

	class RotationMatrix
	{
		// Stored as a column-major 3x3 matrix
	public:
		static const uint8_t IDENTITY = 0x4u;
		//static const uint8_t IDENTITY = 0x34u;
		RotationMatrix(uint8_t packed_matrix)
		{
			unsigned int first_row_index = packed_matrix & 0x3u;
			unsigned int second_row_index = (packed_matrix >> 2) & 0x3u;
			unsigned int third_row_index = ~(first_row_index | second_row_index) & 0x3u;
			if (first_row_index == 0x3u || second_row_index == 0x3u || first_row_index == second_row_index)
			{
				std::cout << (first_row_index == 0x3u) << std::endl;
				std::cout << (second_row_index == 0x3u) << std::endl;
				std::cout << (first_row_index == second_row_index) << std::endl;
				std::cout << first_row_index << std::endl;
				throw std::runtime_error("Invalid rotation matrix initializer!");
			}
			int first_row_value = ((packed_matrix & 0x10u) == 0x10u) ? -1 : 1;
			int second_row_value = ((packed_matrix & 0x20u) == 0x20u) ? -1 : 1;
			int third_row_value = ((packed_matrix & 0x40u) == 0x40u) ? -1 : 1;
			/*
			matrix_[first_row_index][0] = first_row_value;
			matrix_[second_row_index][1] = second_row_value;
			matrix_[third_row_index][2] = third_row_value;
			*/
			matrix_[0][first_row_index] = first_row_value;
			matrix_[1][second_row_index] = second_row_value;
			matrix_[2][third_row_index] = third_row_value;
		}
		RotationMatrix() : RotationMatrix(IDENTITY) {}
		RotationMatrix(const RotationMatrix &other) { copy(other); }
		RotationMatrix &operator=(const RotationMatrix &other) { copy(other); return *this; }
		friend RotationMatrix operator*(const RotationMatrix &left, const RotationMatrix &right);

		RotationMatrix transpose()
		{
			RotationMatrix new_matrix;
			for (unsigned int i = 0; i < 3; i++)
			{
				for (unsigned int j = 0; j < 3; j++)
				{
					new_matrix.matrix_[i][j] = matrix_[j][i];
				}
			}
			return new_matrix;
		}
		RotationMatrix inverse()
		{
			// Fun fact! .vox rotation matrices are always orthogonal, so the inverse is equal to the transpose.
			return transpose();
		}

		void copy(const RotationMatrix &other)
		{
			for (unsigned int i = 0; i < 3; i++)
			{
				for (unsigned int j = 0; j < 3; j++)
				{
					matrix_[i][j] = other.matrix_[i][j];
				}
			}
		}
		void transformVec3(int32_t *vec3)
		{
			int32_t return_vec3[3];
			for (unsigned int mat_col = 0; mat_col < 3; mat_col++)
			{
				return_vec3[mat_col] = 0;
				for (unsigned int i = 0; i < 3; i++)
				{
					return_vec3[mat_col] += vec3[i]*matrix_[mat_col][i];
				}
			}
			vec3[0] = return_vec3[0];
			vec3[1] = return_vec3[1];
			vec3[2] = return_vec3[2];
			return;
		}
		int columnSign(int col)
		{
			for (unsigned int i = 0; i < 3; i++)
			{
				if (matrix_[col][i] != 0) return matrix_[col][i];
			}
			throw std::runtime_error("Encountered invalid rotation matrix!");
		}
		void print()
		{
			for (unsigned int row = 0; row < 3; row++)
			{
				std::cout << "| ";
				for (unsigned int col = 0; col < 3; col++)
				{
					std::cout << matrix_[col][row] << " ";
				}
				std::cout << "|" << std::endl;
			}
		}
	private:
		int matrix_[3][3] = { {0, 0, 0}, {0, 0, 0}, {0, 0, 0} };
	};
private:
		};


/*
VoxHandler::RotationMatrix operator*(const VoxHandler::RotationMatrix &left, VoxHandler::RotationMatrix &right)
{
	VoxHandler::RotationMatrix return_matrix;
	for (unsigned int row = 0; row < 3; row++)
	{
		for (unsigned int col = 0; col < 3; col++)
		{
			return_matrix.matrix_[row][col] = 0;
			for (unsigned int i = 0; i < 3; i++)
			{
				return_matrix.matrix_[row][col] += left.matrix_[row][i] * right.matrix_[i][col];
			}
		}
	}
	return return_matrix;
}
*/

} // namespace Anthrax

#endif // TRANSFORM_HPP
