/* ---------------------------------------------------------------- *\
 * gltf_handler.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-11-09
\* ---------------------------------------------------------------- */

#ifndef GLTF_HANDLER_HPP
#define GLTF_HANDLER_HPP

/*
#define SIGNED_BYTE 5120
#define UNSIGNED_BYTE 5121
#define SIGNED_SHORT 5122
#define UNSIGNED_SHORT 5123
#define UNSIGNED_INT 5125
#define FLOAT 5126
*/

#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>

#include "tools.hpp"

#include "json/json.h"
#include "material.hpp"
#include "world.hpp"
#include "transform.hpp"

namespace Anthrax
{

class World;

class GltfHandler
{
public:
	GltfHandler();
	GltfHandler(World *world);
	~GltfHandler();
private:
	enum Type
	{
		UNKNOWN,
		GLTF,
		GLB
	};
	Type file_type_;
	enum AccessorDataType
	{
		SIGNED_BYTE = 5120,
		UNSIGNED_BYTE = 5121,
		SIGNED_SHORT = 5122,
		UNSIGNED_SHORT = 5123,
		UNSIGNED_INT = 5125,
		FLOAT = 5126
	};
	std::ifstream gltffile_;
	std::string gltfdir_;
	void skipData(size_t num_bytes);
	void readData(void *data, size_t num_bytes);
	uint32_t readUint32();
	void loadFile();
	void loadGltf();
	void loadGlb();
	void loadJsonChunk(uint32_t chunk_length);
	void loadBinChunk(uint32_t chunk_length);

	Json::Value json_;
	std::vector<char> bin_;

	World *world_;
	float unit_length_mm_ = 1000.0; // glTF standard unit length is 1 meter (1000 millimeters)
};

} // namespace Anthrax

#endif // GLTF_HANDLER_HPP
