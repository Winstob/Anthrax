/* ---------------------------------------------------------------- *\
 * voxelizer.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-11-11
\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\
 * Voxelizer: A tool that takes in a standardized mesh and creates
 * a voxelized version of it.
 * Usage: feed in textures, then feed in triangles.
\* ---------------------------------------------------------------- */

#ifndef VOXELIZER_HPP
#define VOXELIZER_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>

#include "tools.hpp"

#include "material.hpp"
#include "mesh.hpp"

namespace Anthrax
{

class Voxelizer
{
public:
	Voxelizer();
	~Voxelizer();
private:
};

} // namespace Anthrax

#endif // VOXELIZER_HPP
