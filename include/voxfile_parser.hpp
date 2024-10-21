/* ---------------------------------------------------------------- *\
 * voxfile_parser.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-09-21
 *
 * VOX files store coordinates in a right-hand system, but with the
 * y and z axes swapped. The VoxfileParser object holds information
 * in the same way, so be aware of this when using.
\* ---------------------------------------------------------------- */

#ifndef VOXFILE_PARSER_HPP
#define VOXFILE_PARSER_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <map>

#include "tools.hpp"

#include "material.hpp"
#include "world.hpp"

namespace Anthrax
{

class World;

class VoxfileParser
{
	class Model;
	class Dict;
	class TransformNode;
	class GroupNode;
	class ShapeNode;
	class SceneNode;
public:
	VoxfileParser() {};
	VoxfileParser(World *world);
	~VoxfileParser();

	Material *getMaterialsPtr() { return materials_; }

	class RotationMatrix
	{
		// Stored as a column-major 3x3 matrix
	public:
		static const uint8_t IDENTITY = 0x4u;
		//static const uint8_t IDENTITY = 0x34u;
		RotationMatrix(uint8_t packed_matrix)
		{
			unsigned int first_row_index = packed_matrix & 0x3u;
			unsigned int second_row_index = (packed_matrix >> 2) & 0x3u;
			unsigned int third_row_index = ~(first_row_index | second_row_index) & 0x3u;
			if (first_row_index == 0x3u || second_row_index == 0x3u || first_row_index == second_row_index)
			{
				std::cout << (first_row_index == 0x3u) << std::endl;
				std::cout << (second_row_index == 0x3u) << std::endl;
				std::cout << (first_row_index == second_row_index) << std::endl;
				std::cout << first_row_index << std::endl;
				throw std::runtime_error("Invalid rotation matrix initializer!");
			}
			int first_row_value = ((packed_matrix & 0x10u) == 0x10u) ? -1 : 1;
			int second_row_value = ((packed_matrix & 0x20u) == 0x20u) ? -1 : 1;
			int third_row_value = ((packed_matrix & 0x40u) == 0x40u) ? -1 : 1;
			/*
			matrix_[first_row_index][0] = first_row_value;
			matrix_[second_row_index][1] = second_row_value;
			matrix_[third_row_index][2] = third_row_value;
			*/
			matrix_[0][first_row_index] = first_row_value;
			matrix_[1][second_row_index] = second_row_value;
			matrix_[2][third_row_index] = third_row_value;
		}
		RotationMatrix() : RotationMatrix(IDENTITY) {}
		RotationMatrix(const RotationMatrix &other) { copy(other); }
		RotationMatrix &operator=(const RotationMatrix &other) { copy(other); return *this; }
		friend RotationMatrix operator*(const RotationMatrix &left, const RotationMatrix &right);

		RotationMatrix transpose()
		{
			RotationMatrix new_matrix;
			for (unsigned int i = 0; i < 3; i++)
			{
				for (unsigned int j = 0; j < 3; j++)
				{
					new_matrix.matrix_[i][j] = matrix_[j][i];
				}
			}
			return new_matrix;
		}
		RotationMatrix inverse()
		{
			// Fun fact! .vox rotation matrices are always orthogonal, so the inverse is equal to the transpose.
			return transpose();
		}

		void copy(const RotationMatrix &other)
		{
			for (unsigned int i = 0; i < 3; i++)
			{
				for (unsigned int j = 0; j < 3; j++)
				{
					matrix_[i][j] = other.matrix_[i][j];
				}
			}
		}
		void transformVec3(int32_t *vec3)
		{
			int32_t return_vec3[3];
			for (unsigned int mat_col = 0; mat_col < 3; mat_col++)
			{
				return_vec3[mat_col] = 0;
				for (unsigned int i = 0; i < 3; i++)
				{
					return_vec3[mat_col] += vec3[i]*matrix_[mat_col][i];
				}
			}
			vec3[0] = return_vec3[0];
			vec3[1] = return_vec3[1];
			vec3[2] = return_vec3[2];
			return;
		}
		int columnSign(int col)
		{
			for (unsigned int i = 0; i < 3; i++)
			{
				if (matrix_[col][i] != 0) return matrix_[col][i];
			}
			throw std::runtime_error("Encountered invalid rotation matrix!");
		}
		void print()
		{
			for (unsigned int row = 0; row < 3; row++)
			{
				std::cout << "| ";
				for (unsigned int col = 0; col < 3; col++)
				{
					std::cout << matrix_[col][row] << " ";
				}
				std::cout << "|" << std::endl;
			}
		}
	private:
		int matrix_[3][3] = { {0, 0, 0}, {0, 0, 0}, {0, 0, 0} };
	};
private:
	std::ifstream voxfile_;
	void skipData(size_t num_bytes);
	void readData(void *data, size_t num_bytes);
	int32_t readInt32();
	void parseFile();
	void parseSizeXyziPair();
	void parseRGBA();
	void parsenTRN();
	void parsenGRP();
	void parsenSHP();
	void parseLAYR();
	void parseMATL();
	void parserOBJ();
	void parserCAM();
	void parseNOTE();
	void parseIMAP();
	Dict parseDict();
	std::string parseString();
	void traverseSceneNode(int32_t id, int32_t x_translation, int32_t y_translation, int32_t z_translation, RotationMatrix rotation);
	void drawSingleModel(int32_t model_id, int32_t x_translation, int32_t y_translation, int32_t z_translation, RotationMatrix rotation);

	std::vector<Model> models_;
	std::vector<SceneNode> scene_nodes_;

	Material materials_[256];
	World *world_;

	bool encountered_rgba_chunk_ = false;




	class Model
	{
	public:
		void allocateMemory()
		{
			data = reinterpret_cast<uint8_t***>(malloc(sizeof(uint8_t**)*size_x));
			for (unsigned int current_x = 0; current_x < size_x; current_x++)
			{
				data[current_x] = reinterpret_cast<uint8_t**>(malloc(sizeof(uint8_t*)*size_y));
				for (unsigned int current_y = 0; current_y < size_y; current_y++)
				{
					data[current_x][current_y] = reinterpret_cast<uint8_t*>(calloc(size_z, sizeof(uint8_t)));
				}
			}
			return;
		}
		Model(unsigned int x, unsigned int y, unsigned int z)
		{
			size_x = x;
			size_y = y;
			size_z = z;
			allocateMemory();
			return;
		}
		~Model()
		{
			for (unsigned int x = 0; x < size_x; x++)
			{
				for (unsigned int y = 0; y < size_y; y++)
				{
					free(data[x][y]);
				}
				free(data[x]);
			}
			free(data);
			return;
		}
		Model(const Model& other) { copy(other); }
		Model& operator=(const Model &other) { copy(other); return *this; }
		void copy(const Model &other)
		{
			size_x = other.size_x;
			size_y = other.size_y;
			size_z = other.size_z;
			allocateMemory();
			for (unsigned int current_x = 0; current_x < size_x; current_x++)
			{
				for (unsigned int current_y = 0; current_y < size_y; current_y++)
				{
					for (unsigned int current_z = 0; current_z < size_z; current_z++)
					data[current_x][current_y][current_z] = other.data[current_x][current_y][current_z];
				}
			}

		}
		uint8_t ***data;
		int32_t size_x, size_y, size_z;
	};

	class Dict
	{
	public:
		Dict(size_t size)
		{
			size_ = size;
		}
		Dict(const Dict &other) { copy(other); }
		Dict& operator=(const Dict &other) { copy(other); return *this; }
		void copy(const Dict &other)
		{
			size_ = other.size_;
			data = other.data;
		}
		Dict() : Dict(0) {}
		size_t size() { return size_; }
		std::map<std::string, std::string> data;
	private:
		size_t size_;
	};

	class TransformNode
	{
	public:
		TransformNode(int32_t child_node_id, int32_t layer_id, int32_t *translation, uint8_t rotation)
		{
			child_node_id_ = child_node_id;
			layer_id_ = layer_id;
			if (translation)
			{
				translation_[0] = translation[0];
				translation_[1] = translation[1];
				translation_[2] = translation[2];
			}
			else
			{
				translation_[0] = 0;
				translation_[1] = 0;
				translation_[2] = 0;
			}
			rotation_ = RotationMatrix(rotation);
		}
		TransformNode() : TransformNode(0, 0, nullptr, RotationMatrix::IDENTITY) {}
		TransformNode(const TransformNode &other) { copy(other); }
		TransformNode &operator=(const TransformNode &other) { copy(other); return *this; }
		void copy(const TransformNode &other)
		{
			child_node_id_ = other.child_node_id_;
			layer_id_ = other.layer_id_;
			translation_[0] = other.translation_[0];
			translation_[1] = other.translation_[1];
			translation_[2] = other.translation_[2];
			rotation_ = other.rotation_;
		}
		int32_t getChildNodeID() { return child_node_id_; }
		int32_t getTranslationX() { return translation_[0]; }
		int32_t getTranslationY() { return translation_[1]; }
		int32_t getTranslationZ() { return translation_[2]; }
		RotationMatrix getRotationMatrix() { return rotation_; }
	private:
		int32_t child_node_id_;
		int32_t layer_id_;
		int32_t translation_[3] = { 0, 0, 0 };
		RotationMatrix rotation_;
	};

	class GroupNode
	{
	public:
		GroupNode(int32_t num_children, int32_t *children_ids)
		{
			children_ids_.resize(num_children);
			children_ids_.assign(children_ids, children_ids + num_children);
		}
		GroupNode() : GroupNode(0, nullptr) {}
		GroupNode(const GroupNode &other) { copy(other); }
		GroupNode &operator=(const GroupNode &other) { copy(other); return *this; }
		void copy(const GroupNode &other)
		{
			children_ids_ = other.children_ids_;
		}
		std::vector<int32_t> getChildrenNodes() { return children_ids_; }
	private:
		std::vector<int32_t> children_ids_;
	};

	class ShapeNode
	{
	public:
		ShapeNode(int32_t num_models, int32_t *model_ids)
		{
			model_ids_.resize(num_models);
			model_ids_.assign(model_ids, model_ids + num_models);
		}
		ShapeNode() : ShapeNode(0, nullptr) {}
		ShapeNode(const ShapeNode &other) { copy(other); }
		ShapeNode &operator=(const ShapeNode &other) { copy(other); return *this; }
		void copy(const ShapeNode &other)
		{
			model_ids_ = other.model_ids_;
		}
		std::vector<int32_t> getModelIDs() { return model_ids_; }
	private:
		std::vector<int32_t> model_ids_;
	};

	class SceneNode
	{
	public:
		typedef enum Type
		{
			UNASSIGNED,
			TRANSFORM_NODE,
			GROUP_NODE,
			SHAPE_NODE
		} Type;
		SceneNode()
		{
			type_ = Type::UNASSIGNED;
		}
		SceneNode(TransformNode node)
		{
			type_ = Type::TRANSFORM_NODE;
			t_node_ = node;
		}
		SceneNode(GroupNode node)
		{
			type_ = Type::GROUP_NODE;
			g_node_ = node;
		}
		SceneNode(ShapeNode node)
		{
			type_ = Type::SHAPE_NODE;
			s_node_ = node;
		}
		SceneNode(const SceneNode &other) { copy(other); }
		SceneNode &operator=(const SceneNode &other) { copy(other); return *this; }
		void copy(const SceneNode &other)
		{
			type_ = other.type_;
			t_node_ = other.t_node_;
			g_node_ = other.g_node_;
			s_node_ = other.s_node_;
		}
		Type type() { return type_; }
		int32_t getChildNodeID()
		{
			if (type_ == TRANSFORM_NODE)
				return t_node_.getChildNodeID();
			else
				throw std::runtime_error("Not a valid SceneNode type - no child node id exists!");
		}
		int32_t getTranslationX()
		{
			if (type_ == TRANSFORM_NODE)
				return t_node_.getTranslationX();
			else
				throw std::runtime_error("Not a valid SceneNode type - no x translation value exists!");
		}
		int32_t getTranslationY()
		{
			if (type_ == TRANSFORM_NODE)
				return t_node_.getTranslationY();
			else
				throw std::runtime_error("Not a valid SceneNode type - no y translation value exists!");
		}
		int32_t getTranslationZ()
		{
			if (type_ == TRANSFORM_NODE)
				return t_node_.getTranslationZ();
			else
				throw std::runtime_error("Not a valid SceneNode type - no z translation value exists!");
		}
		RotationMatrix getRotationMatrix()
		{
			if (type_ == TRANSFORM_NODE)
				return t_node_.getRotationMatrix();
			else
				throw std::runtime_error("Not a valid SceneNode type - no rotation matrix exists!");
		}

		std::vector<int32_t> getChildrenNodes()
		{
			if (type_ == GROUP_NODE)
				return g_node_.getChildrenNodes();
			else
				throw std::runtime_error("Not a valid SceneNode type - no children node list exists!");
		}
		std::vector<int32_t> getModels()
		{
			if (type_ == SHAPE_NODE)
				return s_node_.getModelIDs();
			else
				throw std::runtime_error("Not a valid SceneNode type - no children node list exists!");
		}
	private:
		Type type_;
		TransformNode t_node_;
		GroupNode g_node_;
		ShapeNode s_node_;
	};


};


/*
VoxfileParser::RotationMatrix operator*(const VoxfileParser::RotationMatrix &left, VoxfileParser::RotationMatrix &right)
{
	VoxfileParser::RotationMatrix return_matrix;
	for (unsigned int row = 0; row < 3; row++)
	{
		for (unsigned int col = 0; col < 3; col++)
		{
			return_matrix.matrix_[row][col] = 0;
			for (unsigned int i = 0; i < 3; i++)
			{
				return_matrix.matrix_[row][col] += left.matrix_[row][i] * right.matrix_[i][col];
			}
		}
	}
	return return_matrix;
}
*/

} // namespace Anthrax

#endif // VOXFILE_PARSER_HPP
