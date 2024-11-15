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

	loadBuffers();
	loadBufferViews();
	loadAccessors();
	//unsigned int offset[3] = { 0, 0, 0 };
	unsigned int offset[3] = { 2048, 2048, 2048 };

	/*
	float t[3] = {10.0, 20.0, 30.0};
	float r[4] = {0.259, 0.0, 0.0, 0.966};
	float s[3] = {2.0, 1.0, 0.5};
	Transform transform(t, r, s);
	transform.print();
	*/
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
	/*
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
	*/
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


void GltfHandler::loadBuffers()
{
	// load all buffers into memory
	unsigned int i = 0;
	while (Json::Value current_buffer = json_["buffers"][i])
	{
		buffers_.push_back(Buffer(this, current_buffer));
		i++;
	}
	std::cout << "Loaded " << i << " buffer(s)" << std::endl;
	return;
}


void GltfHandler::loadBufferViews()
{
	// load all bufferviews into memory
	unsigned int i = 0;
	while (Json::Value current_buffer_view = json_["bufferViews"][i])
	{
		buffer_views_.push_back(BufferView(this, current_buffer_view));
		i++;
	}
	std::cout << "Loaded " << i << " bufferView(s)" << std::endl;
	return;
}


void GltfHandler::loadAccessors()
{
	// load all bufferviews into memory
	unsigned int i = 0;
	while (Json::Value current_accessor = json_["accessors"][i])
	{
		accessors_.push_back(Accessor(this, current_accessor));
		i++;
	}
	std::cout << "Loaded " << i << " accessor(s)" << std::endl;
	return;
}


/* ---------------------------------------------------------------- *\
 * Buffer implementation
\* ---------------------------------------------------------------- */

GltfHandler::Buffer::Buffer(GltfHandler *parent, Json::Value json_entry)
{
	std::string uri = json_entry["uri"].asString();
	int buffer_size = json_entry["byteLength"].asInt();

	if (initialized_)
	{
		throw std::runtime_error("Buffer already filled!");
	}
	data_ = (unsigned char*)malloc(buffer_size);
	// TODO: currently this assumes non-embedded data URI's. Need to support data URI's as well.
	// TODO: also assumes gltf (not glb). May not be an issue anyway? who knows
	std::string full_filepath = parent->gltfdir_ + "/" + uri;
	if (!(std::filesystem::exists(full_filepath) && std::filesystem::is_regular_file(full_filepath)))
	{
		throw std::runtime_error("Couldn't resolve URI!");
	}

	std::ifstream file(full_filepath, std::ios::binary);
	if (!file.read(reinterpret_cast<char*>(data_), buffer_size))
	{
		throw std::runtime_error("Failed to read binary file!");
	}
	file.close();

	initialized_ = true;
	return;
}


GltfHandler::Buffer::~Buffer()
{
	free(data_);
	return;
}

/* ---------------------------------------------------------------- *\
 * BufferView implementation
\* ---------------------------------------------------------------- */

GltfHandler::BufferView::BufferView(GltfHandler *parent, Json::Value json_entry)
{
	int buffer_index = json_entry["buffer"].asInt();
	buffer_ = &(parent->buffers_[buffer_index]);
	byte_offset_ = json_entry["bufferOffset"].asInt();
	byte_length_ = json_entry["byteLength"].asInt();
	if (json_entry["byteStride"])
	{
		byte_stride_ = json_entry["byteStride"].asInt();
	}
	target_ = json_entry["target"].asInt();
	return;
}


GltfHandler::BufferView::~BufferView()
{
	return;
}

/* ---------------------------------------------------------------- *\
 * Accessor implementation
\* ---------------------------------------------------------------- */

GltfHandler::Accessor::Accessor(GltfHandler *parent, Json::Value json_entry)
{
	int buffer_view_index = json_entry["bufferView"].asInt();
	buffer_view_ = &(parent->buffer_views_[buffer_view_index]);
	byte_offset_ = json_entry["byteOffset"].asInt();
	component_type_ = json_entry["componentType"].asInt();
	count_ = json_entry["count"].asInt();
	type_ = json_entry["type"].asString();
	// optional min/max
	min_ = json_entry["min"];
	max_ = json_entry["max"];
	if (json_entry["values"])
	{
		throw std::runtime_error("glTF sparse accessors not yet implemented!");
	}
}


GltfHandler::Accessor::~Accessor()
{
	return;
}

} // namespace Anthrax
