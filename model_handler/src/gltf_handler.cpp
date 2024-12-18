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

GltfHandler::GltfHandler()
{
	// TODO: endianness correction?
	std::string input_file_location = "";
	//input_file_location = "models/glb/sponza.glb";
	input_file_location = "models/gltf/sponza";
	//input_file_location = "models/gltf/suzanne";

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
	loadTextures();
	loadMaterials();
	constructMesh();
	return;
}


GltfHandler::~GltfHandler()
{
	gltffile_.close();
}


void GltfHandler::constructMesh()
{
	int scene_index = json_["scene"].asInt();
	Json::Value scene = json_["scenes"][scene_index];
	for (unsigned int i = 0; scene["nodes"][i]; i++)
	{
		int node_index = scene["nodes"][i].asInt();
		Json::Value node = json_["nodes"][node_index];
		processNode(Node(this, node));
	}

	return;
}


void GltfHandler::processNode(Node node)
{
	// extract node elements
	Transform new_transform;
	if (node.json_["matrix"])
	{
		float matrix[4][4];
		for (unsigned int col; col < 4; col++)
		{
			for (unsigned int row; row < 4; row++)
				matrix[col][row] = node.json_["matrix"][col*4+row].asFloat();
		}
		new_transform = Transform(matrix);
	}
	else
	{
		float translation[3] = {0.0, 0.0, 0.0};
		float rotation[4] = {0.0, 0.0, 0.0, 1.0};
		float scale[3] = {1.0, 1.0, 1.0};
		if (node.json_["translation"])
		{
			for (unsigned int i = 0; i < 3; i++)
				translation[i] = node.json_["translation"][i].asFloat();
		}
		if (node.json_["rotation"])
		{
			for (unsigned int i = 0; i < 4; i++)
				rotation[i] = node.json_["rotation"][i].asFloat();
		}
		if (node.json_["scale"])
		{
			for (unsigned int i = 0; i < 3; i++)
				scale[i] = node.json_["scale"][i].asFloat();
		}
		new_transform = Transform(translation, rotation, scale);
	}
	node.applyTransform(new_transform);

	// then loop throgh all children, processNode(child)
	for (unsigned int i = 0; node.json_["children"][i]; i++)
	{
		processNode(node.getChild(i));
	}

	// now actually process this node
	if (node.json_["mesh"])
	{
		insertMesh(node);
	}
	return;
}


void GltfHandler::insertMesh(Node node)
{
	Json::Value mesh_json = json_["meshes"][node.json_["mesh"].asInt()];
	for (unsigned int i = 0; mesh_json["primitives"][i]; i++)
	{
		Json::Value primitive = mesh_json["primitives"][i];
		if (!primitive["attributes"]["POSITION"])
		{
			throw std::runtime_error("Encountered mesh primitive with no POSITION attribute!");
		}
		Accessor *positions_accessor = &(accessors_[primitive["attributes"]["POSITION"].asInt()]);
		if (positions_accessor->getType().first != Accessor::FLOAT)
		{
			throw std::runtime_error("POSITION attribute should be a FLOAT!");
		}

		// TODO: do this
		int mode = TRIANGLES;
		if (primitive["mode"]) mode = primitive["mode"].asInt();
		if (mode != TRIANGLES)
		{
			throw std::runtime_error("Non-triangular primitive modes not yet implemented!");
		}

		if (primitive["indices"])
		{
			// use indexed geometry
			Accessor *indices_accessor = &(accessors_[primitive["indices"].asInt()]);
			if (indices_accessor->getType().second != "SCALAR")
			{
				throw std::runtime_error("indices accessor is not a SCALAR!");
			}
			for (unsigned int j = 0; j < indices_accessor->size()/3; j++)
			{
				// obtain the vertex indices for the next 3 vertices (triangle)
				unsigned int vertex_indices[3];
				for (unsigned int current_vertex_index = 0; current_vertex_index < 3; current_vertex_index++)
				{
					switch (indices_accessor->getType().first)
					{
						case Accessor::SIGNED_SHORT:
							vertex_indices[current_vertex_index] = static_cast<unsigned int>(std::any_cast<signed short>((*indices_accessor)[j*3+current_vertex_index]));
							break;
						case Accessor::UNSIGNED_SHORT:
							vertex_indices[current_vertex_index] = static_cast<unsigned int>(std::any_cast<unsigned short>((*indices_accessor)[j*3+current_vertex_index]));
							break;
						case Accessor::UNSIGNED_INT:
							vertex_indices[current_vertex_index] = std::any_cast<unsigned int>((*indices_accessor)[j*3+current_vertex_index]);
							break;
						default:
							throw std::runtime_error("Invalid incides accessor component_type!");
					}
				}
				// find the vertex positions
				if (positions_accessor->getType().second != "VEC3")
				{
					throw std::runtime_error("POSITION attribute should be a VEC3!");
				}
				// positions_accessor must be a FLOAT VEC3 - this should have already been verified.
				float vertex_positions[3][3];
				for (unsigned int current_vertex_index = 0; current_vertex_index < 3; current_vertex_index++)
				{
					std::vector<float> vertex = std::any_cast<std::vector<float>>((*positions_accessor)[vertex_indices[current_vertex_index]]);
					vertex = node.transform(vertex); // don't forget to apply the transform!
					memcpy(vertex_positions[current_vertex_index], vertex.data(), sizeof(float)*3);
				}
				/*
				std::cout << std::endl << vertex_positions[0][0] << ", " << vertex_positions[0][1] << ", " << vertex_positions[0][2];
				std::cout << std::endl << vertex_positions[1][0] << ", " << vertex_positions[1][1] << ", " << vertex_positions[1][2];
				std::cout << std::endl << vertex_positions[2][0] << ", " << vertex_positions[2][1] << ", " << vertex_positions[2][2] << std::endl;
				*/
				// get texture information
				int texture_id = (gltf_materials_[primitive["material"].asInt()]).getTextureId();
				Accessor *texcoord_accessor = &(accessors_[primitive["attributes"]["TEXCOORD_0"].asInt()]);
				if (texcoord_accessor->getType().first != Accessor::FLOAT
						|| texcoord_accessor->getType().second != "VEC2")
				{
					throw std::runtime_error("TEXCOORD accessor has incorrect type!");
				}
				float texcoords[3][2];
				for (unsigned int current_vertex_index = 0; current_vertex_index < 3; current_vertex_index++)
				{
					std::vector<float> texcoord = std::any_cast<std::vector<float>>((*texcoord_accessor)[vertex_indices[current_vertex_index]]);
					memcpy(texcoords[current_vertex_index], texcoord.data(), sizeof(float)*2);
				}
				/*
				std::cout << std::endl << texcoords[0][0] << ", " << texcoords[0][1];
				std::cout << std::endl << texcoords[1][0] << ", " << texcoords[1][1];
				std::cout << std::endl << texcoords[2][0] << ", " << texcoords[2][1] << std::endl;;
				*/
				// add the triangle to the mesh
				//mesh_.addTriangle(vertex_positions);
				mesh_.addTriangle(vertex_positions, texture_id, texcoords);
			}
		}
		else
		{
			throw std::runtime_error("Non-indexed geometry not yet implemented!");
		}
	}
	/*
	Json::Value mesh_json = json_["meshes"][node.json_["mesh"].asInt()];
	for (unsigned int i = 0; mesh_json["primitives"][i]; i++)
	{
		Json::Value primitive = mesh_json["primitives"][i];
		float *positions = nullptr;
		int *indices = nullptr;
		size_t positions_num_bytes = 0;
		size_t indices_num_bytes = 0;
		size_t positions_size = 0;
		size_t indices_size = 0;
		bool indexed_geometry = false;
		if (primitive["indices"])
		{
			indexed_geometry = true;
			indices_num_bytes = accessors_[primitive["indices"].asInt()].retrieve(reinterpret_cast<void*>(indices));
			indices_size = indices_num_bytes / sizeof(int);
		}
		if (!primitive["attributes"]["POSITION"])
		{
			std::cout << i << std::endl;
			throw std::runtime_error("Encountered mesh primitive with no POSITION attribute!");
		}
		positions_num_bytes = accessors_[primitive["attributes"]["POSITION"].asFloat()].retrieve(reinterpret_cast<void*>(positions)); // positions is treated as a flat array here? maybe not the best idea
		positions_size = positions_num_bytes / sizeof(float);

		if (indexed_geometry)
		{
			for (unsigned int j = 0; j < indices_size/3; j++)
			{
				// loop through set of indices (set of 3 ints)
				float vertices[3][3];
				for (unsigned int k = 0; k < 3; k++)
				{
					// get each 3-dimensional vertex
					vertices[k][0] = positions[indices[j*3+k]*3+0];
					vertices[k][1] = positions[indices[j*3+k]*3+1];
					vertices[k][2] = positions[indices[j*3+k]*3+2];
				}
				mesh_.addTriangle(vertices);
			}
		}
		else
		{
			for (unsigned int j = 0; j < positions_size/9; j++)
			{
				float vertices[3][3];
				for (unsigned int k = 0; k < 3; k++)
				{
					vertices[k][0] = positions[j*9+k*3+0];
					vertices[k][1] = positions[j*9+k*3+1];
					vertices[k][2] = positions[j*9+k*3+2];
				}
				mesh_.addTriangle(vertices);
			}
		}


		free(positions);
		if (indexed_geometry) free(indices);
	}
	*/
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


void GltfHandler::loadTextures()
{
	// load all textures into the Mesh object
	// first load all images
	if (texture_ids_.size() != 0)
	{
		throw std::runtime_error("Textures are already loaded!");
	}
	unsigned int i = 0;
	std::vector<int> image_ids;
	while (Json::Value current_image = json_["images"][i])
	{
		std::string uri = current_image["uri"].asString();
		std::string full_filepath = gltfdir_ + "/" + uri;
		int image_id = mesh_.addImage(full_filepath);
		image_ids.push_back(image_id);
		i++;
	}
	// load samplers
	i = 0;
	std::vector<int> sampler_ids;
	while (Json::Value current_sampler = json_["samplers"][i])
	{
		// TODO: do samplers
		int wrap_s = current_sampler["wrapS"].asInt();
		int wrap_t = current_sampler["wrapT"].asInt();
		int sampler_wrap_s, sampler_wrap_t;
		switch(wrap_s)
		{
			case CLAMP_TO_EDGE:
				sampler_wrap_s = Mesh::Sampler::CLAMP_TO_EDGE;
				break;
			case REPEAT:
				sampler_wrap_s = Mesh::Sampler::REPEAT;
				break;
			case MIRRORED_REPEAT:
				sampler_wrap_s = Mesh::Sampler::MIRRORED_REPEAT;
				break;
		}
		switch(wrap_t)
		{
			case CLAMP_TO_EDGE:
				sampler_wrap_t = Mesh::Sampler::CLAMP_TO_EDGE;
				break;
			case REPEAT:
				sampler_wrap_t = Mesh::Sampler::REPEAT;
				break;
			case MIRRORED_REPEAT:
				sampler_wrap_t = Mesh::Sampler::MIRRORED_REPEAT;
				break;
		}
		sampler_ids.push_back(mesh_.addSampler(sampler_wrap_s, sampler_wrap_t));
		i++;
	}
	// load textures
	i = 0;
	while (Json::Value current_texture = json_["textures"][i])
	{
		int image_index = current_texture["source"].asInt();
		int sampler_index = current_texture["sampler"].asInt();
		texture_ids_.push_back(mesh_.addTexture(image_ids[image_index], sampler_ids[sampler_index]));
		i++;
	}
	std::cout << "Loaded " << i << " texture(s)" << std::endl;
	return;
}


void GltfHandler::loadMaterials()
{
	// load all materials into memory
	unsigned int i = 0;
	while (Json::Value current_material = json_["materials"][i])
	{
		gltf_materials_.push_back(GltfMaterial(this, current_material));
		i++;
	}
	std::cout << "Loaded " << i << " material(s)" << std::endl;
	return;
}

/* ---------------------------------------------------------------- *\
 * Buffer implementation
\* ---------------------------------------------------------------- */

GltfHandler::Buffer::Buffer(GltfHandler *parent, Json::Value json_entry)
{
	std::string uri = json_entry["uri"].asString();

	if (initialized_)
	{
		throw std::runtime_error("Buffer already filled!");
	}
	size_ = json_entry["byteLength"].asInt();
	data_ = reinterpret_cast<unsigned char*>(malloc(size_));
	// TODO: currently this assumes non-embedded data URI's. Need to support data URI's as well.
	// TODO: also assumes gltf (not glb). May not be an issue anyway? who knows
	std::string full_filepath = parent->gltfdir_ + "/" + uri;
	if (!(std::filesystem::exists(full_filepath) && std::filesystem::is_regular_file(full_filepath)))
	{
		throw std::runtime_error("Couldn't resolve URI!");
	}

	std::ifstream file(full_filepath, std::ios::binary);
	if (!file.read(reinterpret_cast<char*>(data_), size_))
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
	size_ = 0;
	initialized_ = false;
	return;
}


void GltfHandler::Buffer::copy(const Buffer &other)
{
	initialized_ = other.initialized_;
	size_ = other.size_;
	data_ = (unsigned char*)malloc(size_);
	memcpy(data_, other.data_, size_);
	return;
}


void GltfHandler::Buffer::read(void *memory, int offset, int num_bytes)
{
	memcpy(memory, &(data_[offset]), num_bytes);
	return;
}

/* ---------------------------------------------------------------- *\
 * BufferView implementation
\* ---------------------------------------------------------------- */

GltfHandler::BufferView::BufferView(GltfHandler *parent, Json::Value json_entry)
{
	int buffer_index = json_entry["buffer"].asInt();
	buffer_ = &(parent->buffers_[buffer_index]);
	byte_offset_ = json_entry["byteOffset"].asInt();
	byte_length_ = json_entry["byteLength"].asInt();
	mode_ = SIMPLE;
	if (json_entry["byteStride"])
	{
		byte_stride_ = json_entry["byteStride"].asInt();
		if (byte_stride_ > 0)
			mode_ = INTERLEAVED;
	}
	target_ = json_entry["target"].asInt();
	return;
}


GltfHandler::BufferView::~BufferView()
{
	return;
}


void GltfHandler::BufferView::read(void* memory, int element_index, int accessor_offset, size_t element_size)
{
	int buffer_offset;
	if (mode_ == INTERLEAVED)
	{
		buffer_offset = element_index*byte_stride_ + accessor_offset;
	}
	else
	{
		buffer_offset = element_size*element_index + accessor_offset;
	}
	buffer_->read(memory, buffer_offset+byte_offset_, element_size);
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
	count_ = json_entry["count"].asInt();

	int component_type = json_entry["componentType"].asInt();
	std::string type = json_entry["type"].asString();
	type_ = Type(component_type, type);
	// optional min/max
	min_ = json_entry["min"];
	max_ = json_entry["max"];
	if (json_entry["sparse"])
	{
		throw std::runtime_error("glTF sparse accessors not yet implemented!");
	}
}


GltfHandler::Accessor::~Accessor()
{
	return;
}


std::any GltfHandler::Accessor::operator[](size_t index)
{
	switch(type_.first)
	{
		case SIGNED_BYTE:
			return readDynamicType<signed char>(index);
		case UNSIGNED_BYTE:
			return readDynamicType<unsigned char>(index);
		case SIGNED_SHORT:
			return readDynamicType<signed short>(index);
		case UNSIGNED_SHORT:
			return readDynamicType<unsigned short>(index);
		case UNSIGNED_INT:
			return readDynamicType<unsigned int>(index);
		case FLOAT:
			return readDynamicType<float>(index);
		default:
			throw std::runtime_error("Unknown accessor type!");
	}
}


template <typename T>
std::any GltfHandler::Accessor::readDynamicType(int index)
{
	if (type_.second == "SCALAR")
	{
		T ret;
		buffer_view_->read(&ret, index, byte_offset_, sizeof(T));
		return ret;
	}
	else if (type_.second == "VEC2")
	{
		std::vector<T> ret(2);
		buffer_view_->read(ret.data(), index, byte_offset_, 2*sizeof(T));
		return ret;
	}
	else if (type_.second == "VEC3")
	{
		std::vector<T> ret(3);
		buffer_view_->read(ret.data(), index, byte_offset_, 3*sizeof(T));
		return ret;
	}
	else
	{
		throw std::runtime_error("Unknown accessor type!");
	}
	return NULL;
}



size_t GltfHandler::Accessor::retrieve(void *memory)
{
	return 0;
	/*
	// NOTE: the allocated memory must be freed by the caller!
	if (memory) throw std::runtime_error("Memory to allocate is not null!");
	int num_bytes_per_component;
	int num_components_per_element;
	switch(component_type_)
	{
		case SIGNED_BYTE:
		case UNSIGNED_BYTE:
			num_bytes_per_component = 1;
			break;
		case SIGNED_SHORT:
		case UNSIGNED_SHORT:
			num_bytes_per_component = 2;
			break;
		case UNSIGNED_INT:
		case FLOAT:
			num_bytes_per_component = 4;
			break;
		default:
			std::cout << component_type_ << std::endl;
			throw std::runtime_error("Unknown component type!");
	}
	if (type_ == "SCALAR")
		num_components_per_element = 1;
	else if (type_ == "VEC3")
		num_components_per_element = 3;
	else
		throw std::runtime_error("Unknown accessor type!");

	// allocate memory
	unsigned char* mem_ptr = reinterpret_cast<unsigned char*>(malloc(num_bytes_per_component*num_components_per_element*count_));

	// fill memory
	for (int i = 0; i < count_; i++)
	{
		int offset = i*num_bytes_per_component*num_components_per_element;
		buffer_view_->read(&(mem_ptr[offset]), i, byte_offset_, num_bytes_per_component*num_components_per_element, num_bytes_per_component*num_components_per_element);
		//read(&(mem_ptr[i*num_bytes_per_component*num_components_per_element]), num_bytes_per_component*num_components_per_element)
	}

	memory = mem_ptr;
	return count_*num_bytes_per_component*num_components_per_element;
	*/
}


void GltfHandler::Accessor::copy(const Accessor &other)
{
	buffer_view_ = other.buffer_view_;
	byte_offset_ = other.byte_offset_;
	count_ = other.count_;
	type_ = other.type_;
	min_ = other.min_;
	max_ = other.max_;
	return;
}


/* ---------------------------------------------------------------- *\
 * Node implementation
\* ---------------------------------------------------------------- */

GltfHandler::Node::~Node()
{
	return;
}


void GltfHandler::Node::copy(const Node &other)
{
	parent_ = other.parent_;
	json_ = other.json_;
	transform_ = other.transform_;
	return;
}


GltfHandler::Node GltfHandler::Node::getChild(int child_index)
{
	Node child_node;
	child_node.copy(*this);
	child_node.json_ = parent_->json_["nodes"][json_["children"][child_index].asInt()];
	return child_node;
}


std::vector<float> GltfHandler::Node::transform(std::vector<float> vec)
{
	if (vec.size() != 3)
	{
		throw std::runtime_error("Tried to transform vector of invalid dimensions!");
	}
	transform_.transformVec3(vec.data());
	return vec;
}

/* ---------------------------------------------------------------- *\
 * GltfMaterial implementation
\* ---------------------------------------------------------------- */

GltfHandler::GltfMaterial::GltfMaterial(GltfHandler *parent, Json::Value json)
{
	parent_ = parent;
	base_color_texture_id_ = -1;
	if (json["pbrMetallicRoughness"])
	{
		if (json["pbrMetallicRoughness"]["baseColorTexture"])
		{
			base_color_texture_id_ = parent->texture_ids_[json["pbrMetallicRoughness"]["baseColorTexture"]["index"].asInt()];
			mode_ = TEXTURED;
		}
	}
	if (base_color_texture_id_ == -1)
	{
		throw std::runtime_error("Non-textured materials not yet implemented!");
	}
	return;
}


GltfHandler::GltfMaterial::~GltfMaterial()
{
}

} // namespace Anthrax
