/* ---------------------------------------------------------------- *\
 * glb_handler.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-11-09
\* ---------------------------------------------------------------- */

#ifndef GLB_HANDLER_HPP
#define GLB_HANDLER_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>

#include "tools.hpp"

#include "json/json.h"
#include "material.hpp"
#include "world.hpp"

namespace Anthrax
{

class World;

class GlbHandler
{
public:
	GlbHandler();
	GlbHandler(World *world);
	~GlbHandler();
private:
	std::ifstream glbfile_;
	void skipData(size_t num_bytes);
	void readData(void *data, size_t num_bytes);
	uint32_t readUint32();
	void parseFile();
	void parseJsonChunk(uint32_t chunk_length);
	void parseBinChunk(uint32_t chunk_length);

	Json::Value json_;
	std::vector<char> bin_;

	World *world_;
	float unit_length_mm_ = 1000.0; // glTF standard unit length is 1 meter (1000 millimeters)
};

} // namespace Anthrax

#endif // GLB_HANDLER_HPP
