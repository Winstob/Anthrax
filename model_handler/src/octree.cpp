/* ---------------------------------------------------------------- *\
 * octree.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-12-12
 *
 * It is important to note that the root node can never be uniform
 * (must always have children) due to the way the indirection pool
 * and voxel type pools are laid out. An octree root node must
 * always have valid children.
\* ---------------------------------------------------------------- */

#include "octree.hpp"

#include <iostream>

namespace Anthrax
{

Octree::Octree(int num_layers)
{
	pool_freelist_ = new Freelist();
	octree_pool_ = new std::vector<OctreeNode>();
	layer_ = num_layers;
	split_mode_ = SplitMode::NORMAL;

	if (pool_freelist_->alloc() != 0)
	{
		throw std::runtime_error("Octree freelist generated incorrectly!");
	}
	octree_pool_->resize(8);
	for (unsigned int i = 0; i < 8; i++)
	{
		(*octree_pool_)[i].indirection = 0;
		(*octree_pool_)[i].voxel_type = 0;
	}

	return;
}


Octree::~Octree()
{
	delete pool_freelist_;
	delete octree_pool_;
	return;
}


void Octree::copy(const Octree& other)
{
	// TODO
	// NOTE: shallow copy
	layer_ = other.layer_;
	split_mode_ = other.split_mode_;

	pool_freelist_ = other.pool_freelist_;
	octree_pool_ = other.octree_pool_;
	return;
}


void Octree::clear()
{
	octree_pool_->resize(8);
	pool_freelist_->clear();
	if (pool_freelist_->alloc() != 0)
	{
		throw std::runtime_error("Octree freelist regenerated incorrectly!");
	}
	for (int i = 0; i < 8; i++)
	{
		(*octree_pool_)[i].indirection = 0;
		(*octree_pool_)[i].voxel_type = 0;
	}
	
	return;
}


void Octree::setVoxel(uint32_t x, uint32_t y, uint32_t z, VoxelTypeElement voxel_type)
{
	return setVoxelAtLayer(x, y, z, voxel_type, 0);
}


/* ---------------------------------------------------------------- *\
 * Set a voxel at a specific layer/precision. The octree is treated
 * as if <layer> was layer 0, and coordinates are adjusted
 * accordingly. Beware that this will remove any octree data at
 * lower layers.
\* ---------------------------------------------------------------- */
void Octree::setVoxelAtLayer(uint32_t x, uint32_t y, uint32_t z,
		VoxelTypeElement voxel_type, int layer)
{
	layer = layer_ - layer - 1;
	if (layer > layer_)
	{
		throw std::runtime_error("Layer must be at most num_layers of octree!");
	}
	IndirectionElement indirection = 0;
	IndirectionElement pool_index;
	for (; layer >= 0; layer--)
	{
		uint32_t tmp_x = x >> layer;
		uint32_t tmp_y = y >> layer;
		uint32_t tmp_z = z >> layer;
		//IndirectionElement child = (x & 1u) | ((y & 1u) << 1) | ((z & 1u) << 2);
		IndirectionElement child = (tmp_x & 1u) | ((tmp_y & 1u) << 1) | ((tmp_z & 1u) << 2);
		pool_index = (indirection << 3) | child;
		if (layer == 0)
			break;

		IndirectionElement next_indirection = (*octree_pool_)[pool_index].indirection;
		if (next_indirection == 0)
		{
			VoxelTypeElement old_voxel_type = (*octree_pool_)[pool_index].voxel_type;
			if (old_voxel_type == voxel_type)
			{
				// Nothing to be done here
				return;
			}
			next_indirection = pool_freelist_->alloc();
			if (octree_pool_->size() < (next_indirection<<3)+8)
			{
				octree_pool_->resize((next_indirection<<3)+8);
			}
			(*octree_pool_)[pool_index].indirection = next_indirection;
			// inherit the child voxel types from the split parent
			for (IndirectionElement child_pool_index = (next_indirection << 3);
			     child_pool_index < (next_indirection << 3)+8;
			     child_pool_index++)
			{
				(*octree_pool_)[child_pool_index].indirection = 0;
				(*octree_pool_)[child_pool_index].voxel_type = old_voxel_type;
			}
		}
		indirection = next_indirection;
	}
	(*octree_pool_)[pool_index].voxel_type = voxel_type;
	// now clear out data at layers underneath this, if needed
	if ((*octree_pool_)[pool_index].indirection != 0)
	{
		// TODO
		(*octree_pool_)[pool_index].indirection = 0;
	}
	// merge if possible
	// TODO

	return;
}


VoxelTypeElement Octree::getVoxel(uint32_t x, uint32_t y, uint32_t z)
{
	return getVoxelAtLayer(x, y, z, 0);
}


/* ---------------------------------------------------------------- *\
 * Similar to setVoxelAtLayer(), the octree is treated as if
 * <layer> was layer 0, and coordinates are adjusted accordingly.
 * In order for this to work properly, material_type_ must be set
 * properly at all layers of this octree.
\* ---------------------------------------------------------------- */
VoxelTypeElement Octree::getVoxelAtLayer(uint32_t x, uint32_t y, uint32_t z, int layer)
{
	layer = layer_ - layer - 1;
	if (layer > layer_)
	{
		throw std::runtime_error("Layer must be at most num_layers of octree!");
	}
	IndirectionElement indirection = 0;
	IndirectionElement pool_index;
	for (; layer >= 0; layer--)
	{
		uint32_t tmp_x = x >> layer;
		uint32_t tmp_y = y >> layer;
		uint32_t tmp_z = z >> layer;
		IndirectionElement child = (tmp_x & 1u) | ((tmp_y & 1u) << 1) | ((tmp_z & 1u) << 2);
		pool_index = (indirection << 3) | child;
		IndirectionElement next_indirection = (*octree_pool_)[pool_index].indirection;

		if (next_indirection == 0 || layer == 0)
		{
			return (*octree_pool_)[pool_index].voxel_type;
		}
		indirection = next_indirection;
	}
	// should never get here
	throw std::runtime_error(std::string("Reached end of ") + __func__ + "!");
	return 0;
}


/* ---------------------------------------------------------------- *\
 * This is a less CPU-intensive merge function. While the normal
 * merge() checks all possible merges in all descendents, this
 * function will only check if each of the children are uniform.
 * If they are not, the children are not descended into and the
 * merge terminates.
\* ---------------------------------------------------------------- */
void Octree::simpleMerge()
{
	// TODO
	return;
	/*
	if (isRoot())
	{
		// the root node can never be uniform due to the way the
		// indirection pool is laid out
		return;
	}
	if (isUniform())
	{
		return;
	}
	// check if children can be merged
	VoxelTypeElement material_type = children_[0].getMaterialType();
	bool can_merge_children = true;
	for (unsigned int i = 0; i < 8; i++)
	{
		if (!children_[i].isUniform()
				|| children_[i].getMaterialType() != material_type)
		{
			can_merge_children = false;
			break;
		}
	}
	if (can_merge_children)
	{
		clear();
		setMaterialType(material_type);
		//std::cout << "Merged! Material type: " << material_type << " | Layer: " << layer_ << std::endl;
		if (parent_)
		{
			parent_->simpleMerge();
		}
	}
	return;
	*/
}


/* ---------------------------------------------------------------- *\
 * Update the approximated material type based on the immediate
 * children
\* ---------------------------------------------------------------- */
void Octree::simpleUpdateLOD()
{
	// TODO
	return;
	/*
	if (isUniform())
	{
		return;
	}
	VoxelTypeElement new_material_type = calculateMaterialTypeFromChildren();
	bool material_type_changed = (new_material_type != getMaterialType());
	setMaterialType(new_material_type);
	if (material_type_changed && parent_)
	{
		parent_->simpleUpdateLOD();
	}
	return;
	*/
}


VoxelTypeElement Octree::calculateMaterialTypeFromChildren()
{
	// TODO
	return 0;
	/*
	if (isUniform())
	{
		return getMaterialType();
	}
	// return the material type of the 0th child
	return children_[0].getMaterialType();
	*/
}


void Octree::mergeOctree(Octree *other, uint32_t x, uint32_t y, uint32_t z)
{
	other->mergeIntoOctree(this, x, y, z);
	return;
}


void Octree::mergeIntoOctreeRecursive(Octree *other, IndirectionElement indirection, int layer, uint32_t x, uint32_t y, uint32_t z)
{
	uint32_t quarter_axis_size;
	uint32_t adder;
	uint32_t subtracter;
	uint32_t upper_end_adder;
	uint32_t lower_end_subtracter;
	if (layer == 1)
	{
		quarter_axis_size = 0;
		adder = 0;
		subtracter = 1;
		upper_end_adder = 0;
		lower_end_subtracter = 1;
	}
	else
	{
		quarter_axis_size = (1u << (layer-2));
		adder = quarter_axis_size;
		subtracter = quarter_axis_size;
		upper_end_adder = (1u << (layer-1))-1;
		lower_end_subtracter = (1u << (layer-1));
	}

	IndirectionElement pool_base_index = indirection << 3;
	for (int child = 0; child < 8; child++)
	{
		if ((*octree_pool_)[pool_base_index+child].indirection == 0)
		{
			uint32_t x_min = (child & 1u) ? (x) : (x - lower_end_subtracter);
			uint32_t x_max = (child & 1u) ? (x + upper_end_adder) : (x);
			uint32_t y_min = (child & 2u) ? (y) : (y - lower_end_subtracter);
			uint32_t y_max = (child & 2u) ? (y + upper_end_adder) : (y);
			uint32_t z_min = (child & 4u) ? (z) : (z - lower_end_subtracter);
			uint32_t z_max = (child & 4u) ? (z + upper_end_adder) : (z);
			other->setVoxelTypeWithinBounds((*octree_pool_)[pool_base_index+child].voxel_type,
					x_min, y_min, z_min,
					x_max, y_max, z_max
					);
		}
		else
		{
			uint32_t child_x, child_y, child_z;
			if (child & 1u)
				child_x = x + adder;
			else
				child_x = x - subtracter;
			if (child & 2u)
				child_y = y + adder;
			else
				child_y = y - subtracter;
			if (child & 4u)
				child_z = z + adder;
			else
				child_z = z - subtracter;
			mergeIntoOctreeRecursive(other, (*octree_pool_)[pool_base_index+child].indirection, layer-1, child_x, child_y, child_z);
		}
	}
	return;
}


void Octree::mergeIntoOctree(Octree *other, uint32_t x, uint32_t y, uint32_t z)
{
	/*
	other->copy(this);
	return;
	other->octree_pool_->resize(octree_pool_->size());
	memcpy(other->octree_pool_->data(), octree_pool_->data(), octree_pool_->size()*sizeof(OctreeNode));
	return;
	*/
	return mergeIntoOctreeRecursive(other, 0, layer_, x, y, z);
}


void Octree::convertToUnsignedLoc(int layer, int32_t x, int32_t y, int32_t z, uint32_t *ux, uint32_t *uy, uint32_t *uz)
{
	uint32_t half_max = 0x1u << (layer-1);
	*ux = (x <= 0) ? (half_max - abs(x)) : (half_max + x);
	*uy = (y <= 0) ? (half_max - abs(y)) : (half_max + y);
	*uz = (z <= 0) ? (half_max - abs(z)) : (half_max + z);
	return;
}


/* ---------------------------------------------------------------- *\
 * inclusive
\* ---------------------------------------------------------------- */
void Octree::setVoxelTypeWithinBounds(VoxelTypeElement voxel_type,
		uint32_t x_min,
		uint32_t y_min,
		uint32_t z_min,
		uint32_t x_max,
		uint32_t y_max,
		uint32_t z_max
		)
{
	/*
	for (int curr_x = x_min; curr_x <= x_max; curr_x++)
	{
		for (int curr_y = y_min; curr_y <= y_max; curr_y++)
		{
			for (int curr_z = z_min; curr_z <= z_max; curr_z++)
			{
				//std::cout << curr_x << "|" << curr_y << "|" << curr_z << std::endl;
				setVoxel(curr_x, curr_y, curr_z, voxel_type);
			}
		}
	}
	return;
	*/
	if (x_min > x_max || y_min > y_max || z_min > z_max)
	{
		throw std::runtime_error("setVoxelTypeWithinBounds(): bounding box min \
				must be less than or equal to bounding box max!");
	}

	// find the bounding box for the largest possible nearby octree node
	uint32_t x_size = x_max - x_min + 1;
	uint32_t y_size = y_max - y_min + 1;
	uint32_t z_size = z_max - z_min + 1;
	uint32_t octree_node_size = min(x_size, min(y_size, z_size));
	int highest_layer = Anthrax::log2(octree_node_size);
	octree_node_size = 0x1u << highest_layer;
	uint32_t new_x_min = roundUpToInterval(x_min, octree_node_size);
	uint32_t new_y_min = roundUpToInterval(y_min, octree_node_size);
	uint32_t new_z_min = roundUpToInterval(z_min, octree_node_size);
	uint32_t new_x_max = new_x_min + octree_node_size - 1;
	uint32_t new_y_max = new_y_min + octree_node_size - 1;
	uint32_t new_z_max = new_z_min + octree_node_size - 1;

	// check if this box matches the largest possible nearby octree node
	if (new_x_min == x_min && new_x_max == x_max &&
			new_y_min == y_min && new_y_max == y_max &&
			new_z_min == z_min && new_z_max == z_max)
	{
		/*
		// bit shifts may be implementation specific for negative numbers?
		//setVoxelAtLayer(new_x_min >> highest_layer, new_y_min >> highest_layer,
		//		new_z_min >> highest_layer, voxel_type, highest_layer);
		uint32_t factor = 1;
		for (int i = 0; i < highest_layer; i++) factor *= 2;
		setVoxelAtLayer(new_x_min/factor, new_y_min/factor, new_z_min/factor,
				voxel_type, highest_layer);
		*/
		setVoxelAtLayer(new_x_min >> highest_layer, new_y_min >> highest_layer, new_z_min >> highest_layer, voxel_type, highest_layer);
		return;
	}

	// go down another layer if we have to (if the boundaries of the currently
	// selected node exceed those of the specified box)
	if (new_x_max > x_max ||
			new_y_max > y_max ||
			new_z_max > z_max)
	{
		highest_layer--;
		octree_node_size = 0x1u << highest_layer;
		new_x_min = roundUpToInterval(x_min, octree_node_size);
		new_y_min = roundUpToInterval(y_min, octree_node_size);
		new_z_min = roundUpToInterval(z_min, octree_node_size);
		new_x_max = new_x_min + octree_node_size - 1;
		new_y_max = new_y_min + octree_node_size - 1;
		new_z_max = new_z_min + octree_node_size - 1;
	}

	// calculate x separators
	std::vector<uint32_t> x_min_separators, x_max_separators;
	if (x_min != new_x_min)
	{
		x_min_separators.push_back(x_min);
		x_max_separators.push_back(new_x_min-1);
	}
	for (uint32_t i = 1; i*octree_node_size <= x_max - new_x_min + 1; i++)
	{
		if (x_min_separators.size() == 0)
			x_min_separators.push_back(x_min);
		else
			x_min_separators.push_back(x_max_separators.back()+1);
		x_max_separators.push_back(x_min_separators.back() + octree_node_size - 1);
	}
	if (x_max_separators.back() != x_max)
	{
		x_min_separators.push_back(x_max_separators.back()+1);
		x_max_separators.push_back(x_max);
	}
	// calculate y separators
	std::vector<uint32_t> y_min_separators, y_max_separators;
	if (y_min != new_y_min)
	{
		y_min_separators.push_back(y_min);
		y_max_separators.push_back(new_y_min-1);
	}
	for (uint32_t i = 1; i*octree_node_size <= y_max - new_y_min + 1; i++)
	{
		if (y_min_separators.size() == 0)
			y_min_separators.push_back(y_min);
		else
			y_min_separators.push_back(y_max_separators.back()+1);
		y_max_separators.push_back(y_min_separators.back() + octree_node_size - 1);
	}
	if (y_max_separators.back() != y_max)
	{
		y_min_separators.push_back(y_max_separators.back()+1);
		y_max_separators.push_back(y_max);
	}
	// calculate z separators
	std::vector<uint32_t> z_min_separators, z_max_separators;
	if (z_min != new_z_min)
	{
		z_min_separators.push_back(z_min);
		z_max_separators.push_back(new_z_min-1);
	}
	for (uint32_t i = 1; i*octree_node_size <= z_max - new_z_min + 1; i++)
	{
		if (z_min_separators.size() == 0)
			z_min_separators.push_back(z_min);
		else
			z_min_separators.push_back(z_max_separators.back()+1);
		z_max_separators.push_back(z_min_separators.back() + octree_node_size - 1);
	}
	if (z_max_separators.back() != z_max)
	{
		z_min_separators.push_back(z_max_separators.back()+1);
		z_max_separators.push_back(z_max);
	}

	// split up the bounding box into smaller boxes
	for (unsigned int z = 0; z < z_min_separators.size(); z++)
	{
		for (unsigned int y = 0; y < y_min_separators.size(); y++)
		{
			for (unsigned int x = 0; x < x_min_separators.size(); x++)
			{
				setVoxelTypeWithinBounds(voxel_type,
						x_min_separators[x],
						y_min_separators[y],
						z_min_separators[z],
						x_max_separators[x],
						y_max_separators[y],
						z_max_separators[z]);
			}
		}
	}
	return;
}


uint32_t Octree::roundUpToInterval(uint32_t val, uint32_t interval)
{
	if (interval == 0)
		return val;

	uint32_t remainder = val % interval;
	return val + remainder;
}

} // namespace Anthrax
