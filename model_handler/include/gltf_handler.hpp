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
#include <any>

#include "tools.hpp"

#include "json/json.h"
#include "material.hpp"
#include "world.hpp"
#include "transform.hpp"
#include "mesh.hpp"

namespace Anthrax
{

class World;

class GltfHandler
{
public:
	GltfHandler();
	GltfHandler(World *world);
	~GltfHandler();
	void constructMesh();
private:
	enum Type
	{
		UNKNOWN,
		GLTF,
		GLB
	};
	Type file_type_;
	enum PrimitiveMode
	{
		POINTS = 0,
		LINES = 1,
		LINE_LOOP = 2,
		LINE_STRIP = 3,
		TRIANGLES = 4,
		TRIANGLE_STRIP = 5,
		TRIANGLE_FAN = 6
	};

	class Buffer
	{
	public:
		Buffer() {}
		Buffer(GltfHandler *parent, Json::Value json_entry);
		~Buffer();
		void copy(const Buffer &other);
		Buffer(const Buffer &other) { copy(other); }
		Buffer &operator=(const Buffer &other) { copy(other); return *this; }
		unsigned char *data() { return data_; }
		void read(void *memory, int offset, int num_bytes);
	private:
		unsigned char *data_ = nullptr;
		size_t size_ = 0;
		bool initialized_ = false;
	};
	class BufferView
	{
	public:
		BufferView() {}
		BufferView(GltfHandler *parent, Json::Value json_entry);
		~BufferView();
		void read(void *memory, int element_index, int accessor_offset, size_t element_size);
		enum Mode
		{
			SIMPLE,
			INTERLEAVED
		};
	private:
		Buffer *buffer_ = nullptr;
		int byte_offset_ = 0;
		int byte_length_ = 0;
		int byte_stride_ = -1;
		int mode_ = SIMPLE;
		int target_ = 0;
	};
	class Accessor
	{
	public:
		Accessor() {}
		Accessor(GltfHandler *parent, Json::Value json_entry);
		~Accessor();
		Accessor(const Accessor &other) { copy(other); }
		Accessor &operator=(const Accessor &other) { copy(other); return *this; }
		void copy(const Accessor &other);
		std::any operator[](size_t index);
		size_t size() { return count_; }
		template <typename T> std::any readDynamicType(int index);
		size_t retrieve(void *memory);
		enum ComponentType
		{
			SIGNED_BYTE = 5120,
			UNSIGNED_BYTE = 5121,
			SIGNED_SHORT = 5122,
			UNSIGNED_SHORT = 5123,
			UNSIGNED_INT = 5125,
			FLOAT = 5126
		};
		typedef std::pair<int, std::string> Type;
		Type getType() { return type_; }
	private:
		BufferView *buffer_view_ = nullptr;
		Type type_;
		int byte_offset_ = 0;
		int count_ = 0;
		//int component_type_;
		//std::string type_ = "";
		Json::Value min_, max_;
	};
	class Node
	{
	public:
		Node() {}
		Node(GltfHandler *parent, Json::Value json) : parent_(parent), json_(json) {}
		~Node();
		void copy(const Node &other);
		Node getChild(int child_index);
		void applyTransform(Transform transform) { transform_ = transform*transform_; } // TODO: swap?
		std::vector<float> transform(std::vector<float> vec3);
		Transform getTransform() { return transform_; }
		Json::Value json_;
	private:
		GltfHandler *parent_;
		Transform transform_;
	};
	std::ifstream gltffile_;
	std::string gltfdir_;
	void processNode(Node node);
	void insertMesh(Node node);
	void skipData(size_t num_bytes);
	void readData(void *data, size_t num_bytes);
	uint32_t readUint32();
	void loadFile();
	void loadGltf();
	void loadGlb();
	void loadJsonChunk(uint32_t chunk_length);
	void loadBinChunk(uint32_t chunk_length);
	void loadBuffers();
	void loadBufferViews();
	void loadAccessors();

	Json::Value json_;
	//std::vector<char> bin_;
	std::vector<Buffer> buffers_;
	std::vector<BufferView> buffer_views_;
	std::vector<Accessor> accessors_;

	Mesh mesh_;
	World *world_;
	float unit_length_mm_ = 1000.0; // glTF standard unit length is 1 meter (1000 millimeters)
};

} // namespace Anthrax

#endif // GLTF_HANDLER_HPP
