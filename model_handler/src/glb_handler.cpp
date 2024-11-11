/* ---------------------------------------------------------------- *\
 * glb_handler.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-11-09
\* ---------------------------------------------------------------- */
#include <sstream> 

#include "glb_handler.hpp"

namespace Anthrax
{

GlbHandler::GlbHandler(World *world)
{
	// TODO: endianness correction?
	world_ = world;
	std::string filename = "models/glb/sponza.glb";

	glbfile_ = std::ifstream(filename, std::ios::binary);
	parseFile();
	//unsigned int offset[3] = { 0, 0, 0 };
	unsigned int offset[3] = { 2048, 2048, 2048 };
	exit(0);
}


GlbHandler::~GlbHandler()
{
	glbfile_.close();
}


void GlbHandler::skipData(size_t num_bytes)
{
	char garbage;
	for (int i = 0; i < num_bytes; i++)
	{
		glbfile_.read(&garbage, 1);
	}
	return;
}


void GlbHandler::readData(void *data, size_t num_bytes)
{
	glbfile_.read(reinterpret_cast<char*>(data), num_bytes);
	return;
}


uint32_t GlbHandler::readUint32()
{
	uint32_t return_data;
	readData(&return_data, sizeof(uint32_t));
	return return_data;
}


void GlbHandler::parseFile()
{
	// parse header
	char magic_string[5];
	magic_string[4] = '\0';
	uint32_t version_number;
	uint32_t file_length;
	glbfile_.read(magic_string, 4);
	if (strcmp(magic_string, "glTF"))
	{
		throw std::runtime_error("Magic string does not match glTF!");
	}
	version_number = readUint32();
	std::cout << "Version " << version_number << std::endl;
	file_length = readUint32();

	char chunk_type[5];
	chunk_type[4] = '\0';
	uint chunk_length;

	// check for JSON chunk
	chunk_length = readUint32();
	glbfile_.read(chunk_type, 4);
	if (strcmp(chunk_type, "JSON"))
	{
		throw std::runtime_error("Expected JSON chunk!");
	}
	std::cout << "Parsing JSON chunk..." << std::endl;
	parseJsonChunk(chunk_length);
	std::cout << "Finished parsing JSON chunk" << std::endl;
	
	// check for BIN chunk
	// TODO: May not always exist? If not, maybe it's a glTF file type?
	chunk_length = readUint32();
	glbfile_.read(chunk_type, 4);
	if (strcmp(chunk_type, "BIN\0"))
	{
		throw std::runtime_error("Expected BIN chunk!");
	}
	std::cout << "Parsing BIN chunk..." << std::endl;
	parseBinChunk(chunk_length);
	std::cout << "Finished parsing BIN chunk" << std::endl;

	return;
}


void GlbHandler::parseJsonChunk(uint32_t chunk_length)
{
	std::string json_str;
	json_str.resize(chunk_length);
	glbfile_.read(json_str.data(), chunk_length);
	Json::Reader reader;
	if (!reader.parse(json_str, json_))
	{
		throw std::runtime_error("Failed to parse JSON chunk!");
	}
	Json::Value scene_index = json_["scene"];
	std::cout << "scene: " << scene_index << std::endl;
	return;
}


void GlbHandler::parseBinChunk(uint32_t chunk_length)
{
	skipData(chunk_length);
	return;
}

} // namespace Anthrax
