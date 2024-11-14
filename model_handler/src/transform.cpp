/* ---------------------------------------------------------------- *\
 * transform.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-11-12
\* ---------------------------------------------------------------- */
#include <sstream> 

#include "transform.hpp"

namespace Anthrax
{

Transform::RotationMatrix operator*(const VoxHandler::RotationMatrix &left, const VoxHandler::RotationMatrix &right)
{
	VoxHandler::RotationMatrix return_matrix;
	for (unsigned int row = 0; row < 3; row++)
	{
		for (unsigned int col = 0; col < 3; col++)
		{
			return_matrix.matrix_[col][row] = 0;
			for (unsigned int i = 0; i < 3; i++)
			{
				return_matrix.matrix_[col][row] += left.matrix_[i][row] * right.matrix_[col][i];
			}
		}
	}
	return return_matrix;
}

} // namespace Anthrax
