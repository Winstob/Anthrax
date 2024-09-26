/* ---------------------------------------------------------------- *\
 * voxfile__parser.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-09-21
\* ---------------------------------------------------------------- */
#include <sstream> 

#include "voxfile_parser.hpp"

namespace Anthrax
{

VoxfileParser::VoxfileParser(World *world)
{
	world_ = world;
	//std::string filename = "vox/teapot.vox";
	//std::string filename = "vox/dragon.vox";
	//std::string filename = "vox/castle.vox";
	//std::string filename = "vox/torus.vox";
	std::string filename = "vox/pieta.vox";
	voxfile_ = std::ifstream(filename);
	parseFile();
	if (scene_nodes_.size() == 0)
	{
		drawSingleModel(0, 2048, 2048, 2048);
	}
	else
	{
		//traverseSceneNode(0, 0, 0, 0);
		traverseSceneNode(0, 2048, 2048, 2048);
	}
}


VoxfileParser::~VoxfileParser()
{
	voxfile_.close();
}


void VoxfileParser::skipData(size_t num_bytes)
{
	char garbage;
	for (int i = 0; i < num_bytes; i++)
	{
		voxfile_.read(&garbage, 1);
	}
	return;
}

void VoxfileParser::readData(void *data, size_t num_bytes)
{
	voxfile_.read(reinterpret_cast<char*>(data), num_bytes);
	return;
}


int32_t VoxfileParser::readInt32()
{
	int32_t return_data;
	readData(&return_data, sizeof(int32_t));
	return return_data;
}


void VoxfileParser::parseFile()
{
	//std::ifstream voxfile_("filename", std::ios::binary);
	//std::ifstream voxfile_(filename);

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
	std::cout << chunk_id << std::endl;
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
			skipData(n+m);
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
		else
		{
			std::cout << "Error: encountered unknown chunk id [" << chunk_id << "]" << std::endl;
			exit(1);
		}
	}

	return;
}


void VoxfileParser::parseSizeXyziPair()
{
	char chunk_id[5];
	chunk_id[4] = '\0';
	int n, m;

	// parse SIZE chunk
	int32_t size_x = readInt32();
	int32_t size_z = readInt32();
	int32_t size_y = readInt32();
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
	uint8_t byte_val;
	for (unsigned int i = 0; i < num_voxels; i++)
	{
		x = 0;
		y = 0;
		z = 0;
		voxfile_.read(reinterpret_cast<char*>(&byte_val), 1);
		x = byte_val;
		voxfile_.read(reinterpret_cast<char*>(&byte_val), 1);
		z = byte_val;
		voxfile_.read(reinterpret_cast<char*>(&byte_val), 1);
		y = byte_val;
		voxfile_.read(reinterpret_cast<char*>(&byte_val), 1);
		color_index = byte_val;
		if (x >= size_x || y >= size_y || z >= size_z)
		{
			throw std::runtime_error("Voxel location exceeds model size!");
		}
		new_model.data[x][y][z] = color_index;
		//world_->setVoxel(x, y, z, color_index);
	}
	models_.push_back(new_model);

	return;
}


void VoxfileParser::parsenTRN()
{
	int32_t node_id = readInt32();
	Dict node_attributes = parseDict();
	int32_t child_node_id = readInt32();
	int32_t reserved_id = readInt32();
	int32_t layer_id = readInt32();
	int32_t num_frames = readInt32();

	/*
	char tmp1[3] = "\0\0";
	for (int i = 0; i < 10000; i++)
	{
		auto file_position = voxfile_.tellg();
		voxfile_.read(tmp1, 2);
		if (tmp1[0] == '_')
			std::cout << tmp1 << std::endl;
		tmp1[0] = '\0';
		tmp1[1] = '\0';
		voxfile_.seekg(file_position);
		voxfile_.ignore(1);
	}
	exit(1);
	*/

	//Dict frame_dicts[num_frames];
	Dict first_frame = parseDict();
	for (int32_t i = 0; i < num_frames-1; i++)
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
			throw std::runtime_error("No [_t] key-value pair in nTRN chunk!");
		*/
	}
	else
	{
		std::string translation_str = first_frame.data["_t"];
		if (sscanf(translation_str.c_str(), "%i %i %i", &translation[0], &translation[2], &translation[1]) != 3)
		{
			throw std::runtime_error("Failed to parse translation string for nTRN chunk!");
		}
	}
	if (node_id >= scene_nodes_.size())
	{
		scene_nodes_.resize(node_id+1);
	}
	//transform_nodes_[node_id] = TransformNode(child_node_id, layer_id, node_attributes, num_frames, frame_dicts);
	//transform_nodes_[node_id] = TransformNode(child_node_id, layer_id, translation);
	if (scene_nodes_[node_id].type() == SceneNode::UNASSIGNED)
	{
		scene_nodes_[node_id] = SceneNode(TransformNode(child_node_id, layer_id, translation));
	}
	else
	{
		std::cout << scene_nodes_[node_id].type() << std::endl;
		throw std::runtime_error("Node ID already assigned!");
	}
	return;
}


void VoxfileParser::parsenGRP()
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


void VoxfileParser::parsenSHP()
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


void VoxfileParser::parseLAYR()
{
	int32_t layer_id = readInt32();
	Dict layer_attribute = parseDict();
	int32_t reserved_id = readInt32();
	// TODO: do something with this LAYR object. can we use it now? store it?
	return;
}


void VoxfileParser::parseMATL()
{
	int32_t material_id = readInt32();
	Dict material_properties = parseDict();
	// TODO: do something with this MATL object. can we use it now? store it?
	return;
}


void VoxfileParser::parserOBJ()
{
	Dict rendering_attributes = parseDict();
	// TODO: do something with this rOBJ object. can we use it now? store it?
	return;
}


void VoxfileParser::parserCAM()
{
	int32_t camera_id = readInt32();
	Dict camera_attribute = parseDict();
	// TODO: do something with this rCAM object. can we use it now? store it?
	return;
}


void VoxfileParser::parseNOTE()
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


VoxfileParser::Dict VoxfileParser::parseDict()
{
	int32_t num_pairs = readInt32();
	Dict dict(num_pairs);
	for (int32_t i = 0; i < num_pairs; i++)
	{
		std::string key = parseString();
		std::string value = parseString();
		dict.data[key] = value;
	}
	return dict;
}


std::string VoxfileParser::parseString()
{
	int32_t size = readInt32();
	char return_str[size+1];
	return_str[size] = '\0';
	readData(return_str, size);
	return std::string(return_str);
}


void VoxfileParser::traverseSceneNode(int32_t scene_node_id, int32_t x_translation, int32_t y_translation, int32_t z_translation)
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
			traverseSceneNode(scene_node.getChildNodeID(),
					x_translation+scene_node.getTranslationX(),
					y_translation+scene_node.getTranslationY(),
					z_translation+scene_node.getTranslationZ());
			break;
		}
		case SceneNode::GROUP_NODE:
		{
			std::vector<int32_t> children_node_ids = scene_node.getChildrenNodes();
			for (unsigned int i = 0; i < children_node_ids.size(); i++)
			{
				traverseSceneNode(children_node_ids[i], x_translation, y_translation, z_translation);
			}
			break;
		}
		case SceneNode::SHAPE_NODE:
		{
			std::vector<int32_t> model_ids = scene_node.getModels();
			for (unsigned int i = 0; i < model_ids.size(); i++)
			{
				drawSingleModel(model_ids[i], x_translation, y_translation, z_translation);
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


void VoxfileParser::drawSingleModel(int32_t model_id, int32_t x_translation, int32_t y_translation, int32_t z_translation)
{
	Model model = models_[model_id];
	for (int x = 0; x < model.size_x; x++)
	{
		for (int y = 0; y < model.size_y; y++)
		{
			for (int z = 0; z < model.size_z; z++)
			{
				uint8_t voxel_value = model.data[x][y][z];
				if (voxel_value != 0)
					world_->setVoxel(x+x_translation, y+y_translation, z+z_translation, voxel_value);
			}
		}
	}
	return;
}

} // namespace Anthrax
