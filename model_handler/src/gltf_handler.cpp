/* ---------------------------------------------------------------- *\
 * gltf_handler.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-11-09
\* ---------------------------------------------------------------- */
#include <sstream> 

#include "gltf_handler.hpp"

#include <filesystem>

namespace Anthrax
{

GltfHandler::GltfHandler(World *world)
{
	// TODO: endianness correction?
	world_ = world;
	std::string input_file_location = "";
	//input_file_location = "models/glb/sponza.glb";
	input_file_location = "models/gltf/sponza";

	file_type_ = Type::UNKNOWN;
	std::string filename = "";

	// check if this is a directory. If it is, make sure it contains a .gltf file
	std::string full_path = std::filesystem::current_path().string() + "/" + input_file_location;
	if (std::filesystem::is_directory(full_path))
	{
		bool found_gltf_file = false;
		for (const auto &entry : std::filesystem::directory_iterator(full_path))
		{
			std::string test_filename = entry.path().string();
			std::string test_file_extension = entry.path().extension().string();
			if (test_file_extension == ".gltf")
			{
				found_gltf_file = true;
				filename = test_filename;
				gltfdir_ = full_path;
				break;
			}
		}
		if (found_gltf_file)
			file_type_ = Type::GLTF;
	}
	else
	{
		// must not be a gltf file directory. Maybe glb?
		if (std::filesystem::is_regular_file(full_path))
		{
			if (std::filesystem::path(full_path).extension().string() == ".glb")
			{
				filename = full_path;
				file_type_ = Type::GLB;
				throw std::runtime_error("GLB file loading not yet implemented!");
			}
		}
	}
	if (file_type_ == Type::UNKNOWN)
	{
		throw std::runtime_error("Model file is not a valid .glb file or .gltf file structure!");
	}

	gltffile_ = std::ifstream(filename, std::ios::binary);
	loadFile();
	//unsigned int offset[3] = { 0, 0, 0 };
	unsigned int offset[3] = { 2048, 2048, 2048 };
	exit(0);
}


GltfHandler::~GltfHandler()
{
	gltffile_.close();
}


void GltfHandler::skipData(size_t num_bytes)
{
	char garbage;
	for (int i = 0; i < num_bytes; i++)
	{
		gltffile_.read(&garbage, 1);
	}
	return;
}


void GltfHandler::readData(void *data, size_t num_bytes)
{
	gltffile_.read(reinterpret_cast<char*>(data), num_bytes);
	return;
}


uint32_t GltfHandler::readUint32()
{
	uint32_t return_data;
	readData(&return_data, sizeof(uint32_t));
	return return_data;
}


void GltfHandler::loadFile()
{
	if (file_type_ == Type::GLTF)
	{
		loadGltf();
	}
	else if (file_type_ == Type::GLB)
	{
		loadGlb();
	}
	return;
}


void GltfHandler::loadGltf()
{
	try
	{
		gltffile_ >> json_;
	}
	catch (const std::exception &e)
	{
		throw std::runtime_error("Failed to parse json file!");
	}
	// need to find and load .bin file too
	std::string bin_filename = json_["buffers"][0]["uri"].asString();
	std::string bin_filepath = gltfdir_ + "/" + bin_filename;
	if (!std::filesystem::exists(bin_filepath))
	{
		throw std::runtime_error("Couldn't find .bin file!");
	}
	int bin_file_length = json_["buffers"][0]["byteLength"].asInt();
	bin_.resize(bin_file_length);
	std::ifstream bin_filestream = std::ifstream(bin_filepath, std::ios::binary);
	bin_filestream.read(bin_.data(), bin_file_length);
	bin_filestream.close();
	return;
}


void GltfHandler::loadGlb()
{
	// TODO: unfinished
	// parse header
	char magic_string[5];
	magic_string[4] = '\0';
	uint32_t version_number;
	uint32_t file_length;
	gltffile_.read(magic_string, 4);
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
	gltffile_.read(chunk_type, 4);
	if (strcmp(chunk_type, "JSON"))
	{
		throw std::runtime_error("Expected JSON chunk!");
	}
	std::cout << "Parsing JSON chunk..." << std::endl;
	loadJsonChunk(chunk_length);
	std::cout << "Finished parsing JSON chunk" << std::endl;
	
	// check for BIN chunk
	// TODO: May not always exist? If not, maybe it's a glTF file type?
	chunk_length = readUint32();
	gltffile_.read(chunk_type, 4);
	if (strcmp(chunk_type, "BIN\0"))
	{
		throw std::runtime_error("Expected BIN chunk!");
	}
	std::cout << "Parsing BIN chunk..." << std::endl;
	loadBinChunk(chunk_length);
	std::cout << "Finished parsing BIN chunk" << std::endl;

	return;
}


void GltfHandler::loadJsonChunk(uint32_t chunk_length)
{
	std::string json_str;
	json_str.resize(chunk_length);
	gltffile_.read(json_str.data(), chunk_length);
	Json::Reader reader;
	if (!reader.parse(json_str, json_))
	{
		throw std::runtime_error("Failed to parse JSON chunk!");
	}
	Json::Value scene_index = json_["scene"];
	std::cout << "scene: " << scene_index << std::endl;
	return;
}


void GltfHandler::loadBinChunk(uint32_t chunk_length)
{
	// TODO: unfinished (glb only)
	skipData(chunk_length);
	return;
}

} // namespace Anthrax
