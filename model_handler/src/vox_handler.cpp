/* ---------------------------------------------------------------- *\
 * vox_handler.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-09-21
\* ---------------------------------------------------------------- */
#include <sstream> 

#include "vox_handler.hpp"

namespace Anthrax
{

VoxHandler::VoxHandler(World *world)
{
	// TODO: endianness correction?
	world_ = world;
	//std::string filename = "vox/teapot.vox";
	//std::string filename = "vox/dragon.vox";
	//std::string filename = "vox/castle.vox";
	std::string filename = "vox/sponza.vox";
	//std::string filename = "vox/nuke.vox";
	//std::string filename = "vox/Church_Of_St_Sophia.vox";
	//std::string filename = "vox/custom.vox";
	//std::string filename = "vox/torus.vox";
	//std::string filename = "vox/pieta.vox";

	voxfile_ = std::ifstream(filename, std::ios::binary);
	parseFile();
	//unsigned int offset[3] = { 0, 0, 0 };
	unsigned int offset[3] = { 2048, 2048, 2048 };
	if (scene_nodes_.size() == 0)
	{
		drawSingleModel(0, offset[0], offset[1], offset[2], RotationMatrix());
	}
	else
	{
		traverseSceneNode(0, offset[0], offset[1], offset[2], RotationMatrix());
	}
}


VoxHandler::~VoxHandler()
{
	voxfile_.close();
}


void VoxHandler::skipData(size_t num_bytes)
{
	char garbage;
	for (int i = 0; i < num_bytes; i++)
	{
		voxfile_.read(&garbage, 1);
	}
	return;
}

void VoxHandler::readData(void *data, size_t num_bytes)
{
	voxfile_.read(reinterpret_cast<char*>(data), num_bytes);
	return;
}


int32_t VoxHandler::readInt32()
{
	int32_t return_data;
	readData(&return_data, sizeof(int32_t));
	return return_data;
}


void VoxHandler::parseFile()
{
	//std::string riff_header_string;
	char riff_header_string[5];
	int version_number;
	riff_header_string[4] = '\0';

	// parse header
	voxfile_.read(riff_header_string, 4);
	if (strcmp(riff_header_string, "VOX "))
	{
		throw std::runtime_error("Invalid RIFF header!");
	}
	version_number = readInt32();
	std::cout << "Version " << version_number << std::endl;

	char chunk_id[5];
	chunk_id[4] = '\0';
	int n, m;

	// parse chunk MAIN
	voxfile_.read(chunk_id, 4);
	if (strcmp(chunk_id, "MAIN"))
	{
		throw std::runtime_error("MAIN chunk not found!");
	}

	n = readInt32();
	m = readInt32();
	std::cout << "MAIN: n = [" << n << "], m = [" << m << "]" << std::endl;

	// search for a PACK chunk
	int num_models = 1;
	auto file_position = voxfile_.tellg();
	voxfile_.read(chunk_id, 4);
	if (!strcmp(chunk_id, "PACK"))
	{
		std::cout << "found PACK chunk" << std::endl;
		num_models = readInt32();
		exit(0);
	}
	else
	{
		voxfile_.seekg(file_position);
	}

	// parse next chunk
	while (voxfile_.peek() != EOF)
	{
		voxfile_.read(chunk_id, 4);
		std::cout << chunk_id << std::endl;
		n = readInt32();
		m = readInt32();
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
			parseSizeXyziPair();
		}
		else if (!strcmp(chunk_id, "XYZI"))
		{
			throw std::runtime_error("Encountered XYZI chunk without corresponding SIZE chunk!");
		}
		else if (!strcmp(chunk_id, "RGBA"))
		{
			parseRGBA();
		}
		else if (!strcmp(chunk_id, "nTRN"))
		{
			parsenTRN();
		}
		else if (!strcmp(chunk_id, "nGRP"))
		{
			parsenGRP();
		}
		else if (!strcmp(chunk_id, "nSHP"))
		{
			parsenSHP();
		}
		else if (!strcmp(chunk_id, "LAYR"))
		{
			parseLAYR();
		}
		else if (!strcmp(chunk_id, "MATL"))
		{
			parseMATL();
		}
		else if (!strcmp(chunk_id, "rOBJ"))
		{
			parserOBJ();
		}
		else if (!strcmp(chunk_id, "rCAM"))
		{
			parserCAM();
		}
		else if (!strcmp(chunk_id, "NOTE"))
		{
			parseNOTE();
		}
		else if (!strcmp(chunk_id, "IMAP"))
		{
			parseIMAP();
		}
		else
		{
			std::cout << "Error: encountered unknown chunk id [" << chunk_id << "]" << std::endl;
			exit(1);
		}
	}

	if (!encountered_rgba_chunk_)
	{
		throw std::runtime_error("RGBA chunk not encountered!");
	}

	return;
}


void VoxHandler::parseSizeXyziPair()
{
	char chunk_id[5];
	chunk_id[4] = '\0';
	int n, m;

	// parse SIZE chunk
	int32_t size_x = readInt32();
	int32_t size_y = readInt32();
	int32_t size_z = readInt32();
	std::cout << "SIZE: x = [" << size_x << "], y = [" << size_y << "], z = [" << size_z << "]" << std::endl;

	// parse XYZI chunk
	voxfile_.read(chunk_id, 4);
	n = readInt32();
	m = readInt32();
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
	int32_t num_voxels = readInt32();
	if (4*num_voxels != n-4)
	{
		throw std::runtime_error("num_voxels does not match size of XYZI chunk data!");
	}

	//models_.push_back(Model(size_x, size_y, size_z));
	Model new_model(size_x, size_y, size_z);
	// read in voxel data
	uint8_t x, y, z, color_index;
	for (unsigned int i = 0; i < num_voxels; i++)
	{
		x = 0;
		y = 0;
		z = 0;
		voxfile_.read(reinterpret_cast<char*>(&x), 1);
		voxfile_.read(reinterpret_cast<char*>(&y), 1);
		voxfile_.read(reinterpret_cast<char*>(&z), 1);
		voxfile_.read(reinterpret_cast<char*>(&color_index), 1);
		if (x >= size_x || y >= size_y || z >= size_z)
		{
			throw std::runtime_error("Voxel location exceeds model size!");
		}
		new_model.data[x][y][z] = color_index;
	}
	models_.push_back(new_model);

	return;
}


void VoxHandler::parseRGBA()
{
	uint8_t r_int, g_int, b_int, a_int;
	float red, green, blue, alpha;
	for (unsigned int i = 0; i < 255; i++)
	{
		voxfile_.read(reinterpret_cast<char*>(&r_int), 1);
		voxfile_.read(reinterpret_cast<char*>(&g_int), 1);
		voxfile_.read(reinterpret_cast<char*>(&b_int), 1);
		voxfile_.read(reinterpret_cast<char*>(&a_int), 1);
		red = float(r_int) / 255.0;
		green = float(g_int) / 255.0;
		blue = float(b_int) / 255.0;
		alpha = float(a_int) / 255.0;
		/*
		if (i == 255)
		{
			std::cout << red << " " << green << " " << blue << " " << alpha << std::endl;
		}
		*/

		materials_[i+1] = Material(red, green, blue, alpha);
	}
	materials_[0] = Material(0.0, 0.0, 0.0, 0.0);
	skipData(4);
	encountered_rgba_chunk_ = true;
	return;
}


void VoxHandler::parsenTRN()
{
	int32_t node_id = readInt32();
	Dict node_attributes = parseDict();
	int32_t child_node_id = readInt32();
	int32_t reserved_id = readInt32();
	std::cout << "reserved id: " << reserved_id << std::endl;
	if (reserved_id != -1)
	{
		throw std::runtime_error("nTRN: reserved id is not -1");
	}
	int32_t layer_id = readInt32();
	int32_t num_frames = readInt32();
	if (num_frames != 1)
	{
		throw std::runtime_error("nTRN: num_frames is not 1");
	}

	//Dict frame_dicts[num_frames];
	Dict first_frame = parseDict();
	for (int32_t i = 1; i < num_frames; i++)
	{
		//parseDict();
		throw std::runtime_error("Haven't implemented skip for multiple frames");
	}
	int32_t translation[3];
	// parse out translation value from the dict string
	if (first_frame.data.find("_t") == first_frame.data.end())
	{
		translation[0] = 0;
		translation[1] = 0;
		translation[2] = 0;
		/*
		if (node_id != 0)
		{
			std::cout << node_id << std::endl;
			throw std::runtime_error("No [_t] key-value pair in nTRN chunk!");
		}
		*/
	}
	else
	{
		std::string translation_str = first_frame.data["_t"];
		if (sscanf(translation_str.c_str(), "%i %i %i", &translation[0], &translation[1], &translation[2]) != 3)
		{
			throw std::runtime_error("Failed to parse translation string for nTRN chunk!");
		}
	}
	uint8_t rotation;
	if (first_frame.data.find("_r") == first_frame.data.end())
	{
		// 0000 0100
		// 1 0 0
		// 0 1 0
		// 0 0 1
		rotation = RotationMatrix::IDENTITY;
	}
	else
	{
		unsigned int rotation_tmp;
		if (sscanf(first_frame.data["_r"].c_str(), "%i", &rotation_tmp) != 1)
		{
			throw std::runtime_error("Failed to parse translation string for nTRN chunk!");
		}
		rotation = (uint8_t)rotation_tmp;
	}
	if (node_id >= scene_nodes_.size())
	{
		scene_nodes_.resize(node_id+1);
	}
	//transform_nodes_[node_id] = TransformNode(child_node_id, layer_id, node_attributes, num_frames, frame_dicts);
	//transform_nodes_[node_id] = TransformNode(child_node_id, layer_id, translation);
	if (scene_nodes_[node_id].type() == SceneNode::UNASSIGNED)
	{
		scene_nodes_[node_id] = SceneNode(TransformNode(child_node_id, layer_id, translation, rotation));
	}
	else
	{
		std::cout << scene_nodes_[node_id].type() << std::endl;
		throw std::runtime_error("Node ID already assigned!");
	}
	return;
}


void VoxHandler::parsenGRP()
{
	int32_t node_id = readInt32();
	Dict node_attributes = parseDict();
	int32_t num_child_nodes = readInt32();

	int32_t child_node_ids[num_child_nodes];
	for (int32_t i = 0; i < num_child_nodes; i++)
	{
		child_node_ids[i] = readInt32();
	}
	if (node_id >= scene_nodes_.size())
	{
		scene_nodes_.resize(node_id+1);
	}
	if (scene_nodes_[node_id].type() == SceneNode::UNASSIGNED)
	{
		scene_nodes_[node_id] = SceneNode(GroupNode(num_child_nodes, child_node_ids));
	}
	else
	{
		std::cout << scene_nodes_[node_id].type() << std::endl;
		throw std::runtime_error("Node ID already assigned!");
	}
	return;
}


void VoxHandler::parsenSHP()
{
	int32_t node_id = readInt32();
	Dict node_attributes = parseDict();
 	int32_t num_models = readInt32();
	int32_t model_ids[num_models];
	for (int32_t i = 0; i < num_models; i++)
	{
		model_ids[i] = readInt32();
		//Dict model_attributes = parseDict();
		parseDict();
	}
	if (node_id >= scene_nodes_.size())
	{
		scene_nodes_.resize(node_id+1);
	}
	if (scene_nodes_[node_id].type() == SceneNode::UNASSIGNED)
	{
		scene_nodes_[node_id] = SceneNode(ShapeNode(num_models, model_ids));
	}
	else
	{
		std::cout << scene_nodes_[node_id].type() << std::endl;
		throw std::runtime_error("Node ID already assigned!");
	}

	return;
}


void VoxHandler::parseLAYR()
{
	int32_t layer_id = readInt32();
	Dict layer_attribute = parseDict();
	int32_t reserved_id = readInt32();
	// TODO: do something with this LAYR object. can we use it now? store it?
	return;
}


void VoxHandler::parseMATL()
{
	int32_t material_id = readInt32();
	Dict material_properties = parseDict();
	// TODO: do something with this MATL object. can we use it now? store it?
	return;
}


void VoxHandler::parserOBJ()
{
	Dict rendering_attributes = parseDict();
	// TODO: do something with this rOBJ object. can we use it now? store it?
	return;
}


void VoxHandler::parserCAM()
{
	int32_t camera_id = readInt32();
	Dict camera_attribute = parseDict();
	// TODO: do something with this rCAM object. can we use it now? store it?
	return;
}


void VoxHandler::parseNOTE()
{
	int32_t num_color_names = readInt32();
	for (int32_t i = 0; i < num_color_names; i++)
	{
		// TODO: do something with these
		parseString();
	}
	// TODO: do something with this NOTE object. can we use it now? store it?
	return;
}


void VoxHandler::parseIMAP()
{
	for (int32_t i = 0; i < 256; i++)
	{
		// TODO: do something with these
		skipData(1);
	}
	// TODO: do something with this IMAP object. can we use it now? store it?
	return;
}


VoxHandler::Dict VoxHandler::parseDict()
{
	int32_t num_pairs = readInt32();
	Dict dict(num_pairs);
	std::cout << num_pairs << std::endl;
	for (int32_t i = 0; i < num_pairs; i++)
	{
		std::string key = parseString();
		std::string value = parseString();
		dict.data[key] = value;
	}
	return dict;
}


std::string VoxHandler::parseString()
{
	int32_t size = readInt32();
	char return_str[size+1];
	return_str[size] = '\0';
	readData(return_str, size);
	return std::string(return_str);
}


void VoxHandler::traverseSceneNode(int32_t scene_node_id, int32_t x_translation, int32_t y_translation, int32_t z_translation, RotationMatrix rotation)
{
	SceneNode scene_node = scene_nodes_[scene_node_id];

	switch (scene_node.type())
	{
		case SceneNode::UNASSIGNED:
		{
			throw std::runtime_error("Referenced scene node is uninitialized!");
			break;
		}
		case SceneNode::TRANSFORM_NODE:
		{
			int32_t current_translation[3] = { x_translation, y_translation, z_translation };
			int32_t next_translation[3] = { scene_node.getTranslationX(), scene_node.getTranslationY(), scene_node.getTranslationZ() };
			rotation.transformVec3(next_translation);
			//rotation = scene_node.getRotationMatrix();
			x_translation = current_translation[0] + next_translation[0];
			y_translation = current_translation[1] + next_translation[1];
			z_translation = current_translation[2] + next_translation[2];
			rotation = scene_node.getRotationMatrix() * rotation;
			traverseSceneNode(scene_node.getChildNodeID(),
					x_translation,
					y_translation,
					z_translation,
					rotation);
			break;
		}
		case SceneNode::GROUP_NODE:
		{
			std::vector<int32_t> children_node_ids = scene_node.getChildrenNodes();
			for (unsigned int i = 0; i < children_node_ids.size(); i++)
			{
				traverseSceneNode(children_node_ids[i], x_translation, y_translation, z_translation, rotation);
			}
			break;
		}
		case SceneNode::SHAPE_NODE:
		{
			std::vector<int32_t> model_ids = scene_node.getModels();
			for (unsigned int i = 0; i < model_ids.size(); i++)
			{
				drawSingleModel(model_ids[i], x_translation, y_translation, z_translation, rotation);
			}
			break;
		}
		default:
		{
			throw std::runtime_error("Scene node has unknown type!");
		}
	}
	return;
}


void VoxHandler::drawSingleModel(int32_t model_id, int32_t x_translation, int32_t y_translation, int32_t z_translation, RotationMatrix rotation)
{
	Model model = models_[model_id];

	int32_t extent[3] = { model.size_x, model.size_y, model.size_z };
	int32_t offset[3] = {0, 0, 0};
	for (unsigned int i = 0; i < 3; i++)
	{
		offset[i] = (rotation.columnSign(i) == -1) ? 1 : 0;
	}

	for (int x = 0; x < model.size_x; x++)
	{
		for (int y = 0; y < model.size_y; y++)
		{
			for (int z = 0; z < model.size_z; z++)
			{
				uint8_t voxel_value = model.data[x][y][z];
				int32_t current_translation[3] = { x-extent[0]/2, y-extent[1]/2, z-extent[2]/2 };
				rotation.transformVec3(current_translation);
				current_translation[0] -= offset[0];
				current_translation[1] -= offset[1];
				current_translation[2] -= offset[2];
				int32_t global_position[3] = { x_translation+current_translation[0], y_translation+current_translation[1], z_translation+current_translation[2] };

				if (voxel_value != 0)
				{
					world_->setVoxel(global_position[1], global_position[2], global_position[0], voxel_value);
				}
			}
		}
	}
	return;
}


VoxHandler::RotationMatrix operator*(const VoxHandler::RotationMatrix &left, const VoxHandler::RotationMatrix &right)
{
	VoxHandler::RotationMatrix return_matrix;
	for (unsigned int row = 0; row < 3; row++)
	{
		for (unsigned int col = 0; col < 3; col++)
		{
			return_matrix.matrix_[col][row] = 0;
			for (unsigned int i = 0; i < 3; i++)
			{
				return_matrix.matrix_[col][row] += left.matrix_[i][row] * right.matrix_[col][i];
			}
		}
	}
	return return_matrix;
}

} // namespace Anthrax
