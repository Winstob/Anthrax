/* ---------------------------------------------------------------- *\
 * voxfile_parser.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-09-21
\* ---------------------------------------------------------------- */
#include "voxfile_parser.hpp"

#include <iostream>

namespace Anthrax
{

VoxfileParser::VoxfileParser(World *world)
{
	world_ = world;
	parseFile("vox/teapot.vox");
	//parseFile("vox/dragon.vox");
	//parseFile("vox/torus.vox");
}


VoxfileParser::~VoxfileParser()
{
}


void VoxfileParser::parseFile(std::string filename)
{
	//std::ifstream voxfile("filename", std::ios::binary);
	std::ifstream voxfile(filename);

	//std::string riff_header_string;
	char riff_header_string[5];
	int version_number;
	riff_header_string[4] = '\0';

	// parse header
	voxfile.read(riff_header_string, 4);
	if (strcmp(riff_header_string, "VOX "))
	{
		throw std::runtime_error("Invalid RIFF header!");
	}
	voxfile.read(reinterpret_cast<char*>(&version_number), 4);

	char chunk_id[5];
	chunk_id[4] = '\0';
	int n, m;
	char garbage;

	// parse chunk MAIN
	voxfile.read(chunk_id, 4);
	if (strcmp(chunk_id, "MAIN"))
	{
		throw std::runtime_error("MAIN chunk not found!");
	}

	voxfile.read(reinterpret_cast<char*>(&n), 4);
	voxfile.read(reinterpret_cast<char*>(&m), 4);
	std::cout << "MAIN: n = [" << n << "], m = [" << m << "]" << std::endl;

	// search for a PACK chunk
	int num_models = 1;
	auto file_position = voxfile.tellg();
	voxfile.read(chunk_id, 4);
	std::cout << chunk_id << std::endl;
	if (!strcmp(chunk_id, "PACK"))
	{
		std::cout << "found PACK chunk" << std::endl;
		voxfile.read(reinterpret_cast<char*>(&num_models), 4);
		exit(0);
	}
	else
	{
		voxfile.seekg(file_position);
	}

	// parse next chunk
	while (voxfile.peek() != EOF)
	{
		voxfile.read(chunk_id, 4);
		voxfile.read(reinterpret_cast<char*>(&n), 4);
		voxfile.read(reinterpret_cast<char*>(&m), 4);
		if (!strcmp(chunk_id, "PACK"))
		{
			throw std::runtime_error("Found second PACK chunk id - invalid format!");
		}
		else if (!strcmp(chunk_id, "SIZE"))
		{
			if (n != 12 || m != 0)
			{
				throw std::runtime_error("Invalid SIZE chunk format!");
			}
			parseSizeXyziPair(voxfile);
		}
		else if (!strcmp(chunk_id, "XYZI"))
		{
			throw std::runtime_error("Encountered XYZI chunk without corresponding SIZE chunk!");
		}
		else if (!strcmp(chunk_id, "RGBA"))
		{
			for (int j = 0; j < n+m; j++)
			{
				voxfile.read(&garbage, 1);
			}
		}
	}

	return;
}


void VoxfileParser::parseSizeXyziPair(std::ifstream &voxfile)
{
	char chunk_id[5];
	chunk_id[4] = '\0';
	int n, m;
	unsigned int size_x, size_y, size_z;

	// parse SIZE chunk
	voxfile.read(reinterpret_cast<char*>(&size_x), 4);
	voxfile.read(reinterpret_cast<char*>(&size_y), 4);
	voxfile.read(reinterpret_cast<char*>(&size_z), 4);
	std::cout << "SIZE: x = [" << size_x << "], y = [" << size_y << "], z = [" << size_z << "]" << std::endl;

	// parse XYZI chunk
	voxfile.read(chunk_id, 4);
	voxfile.read(reinterpret_cast<char*>(&n), 4);
	voxfile.read(reinterpret_cast<char*>(&m), 4);
	/*
	std::cout << std::endl;
	std::cout << chunk_id << std::endl;
	std::cout << n << std::endl;
	std::cout << m << std::endl;
	*/
	if (strcmp(chunk_id, "XYZI"))
	{
		throw std::runtime_error("SIZE chunk not followed by XYZI chunk!");
	}
	if (m > 0)
	{
		throw std::runtime_error("XYZI chunk has children - currently unsupported!");
	}
	unsigned int num_voxels;
	voxfile.read(reinterpret_cast<char*>(&num_voxels), 4);
	if (4*num_voxels != n-4)
	{
		throw std::runtime_error("num_voxels does not match size of XYZI chunk data!");
	}
	// read in voxel data
	unsigned int x, y, z, color_index;
	uint8_t byte_val;
	for (unsigned int i = 0; i < num_voxels; i++)
	{
		x = 0;
		y = 0;
		z = 0;
		voxfile.read(reinterpret_cast<char*>(&byte_val), 1);
		x = byte_val;
		voxfile.read(reinterpret_cast<char*>(&byte_val), 1);
		z = byte_val;
		voxfile.read(reinterpret_cast<char*>(&byte_val), 1);
		y = byte_val;
		voxfile.read(reinterpret_cast<char*>(&byte_val), 1);
		color_index = byte_val;
		// TODO: remove
		x += 2048;
		y += 2048;
		z += 2048;
		world_->setVoxel(x, y, z, color_index);
	}

	return;
}


} // namespace Anthrax
