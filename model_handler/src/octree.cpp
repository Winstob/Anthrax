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

Octree::Octree(Octree *parent, int layer, int this_child_index)
{
	parent_ = parent;
	layer_ = layer;
	this_child_index_ = this_child_index;

	if (isRoot())
	{
		// This is the root octree node - need to set up buffers
		pool_freelist_ = new Freelist();
		octree_pool_ = new std::vector<OctreeNode>();
		split_mode_ = new SplitMode(SPLIT_MODE_NORMAL);
	}
	else
	{
		pool_freelist_ = parent_->pool_freelist_;
		octree_pool_ = parent_->octree_pool_;
		split_mode_ = parent_->split_mode_;
	}
	pool_index_ = pool_freelist_->alloc();
	if (isRoot() && pool_index_ != 0)
	{
		throw std::runtime_error("Pool index of Octree root node is not 0!");
	}
	// Ensure pools have sufficient sizes
	if (octree_pool_->size() <= pool_index_*8)
	{
		octree_pool_->resize((pool_index_+1)*8);
	}

	if (!isRoot())
	{
		//parent_->setIndirection(this_child_index_, pool_index_);
		setIndirection(this_child_index_, pool_index_);
	}

	setUniformity(true);
	setMaterialType(0);
	children_ = nullptr;

	if (isRoot())
	{
		split();
	}
	return;
}


Octree::~Octree()
{
	if (children_)
	{
		for (unsigned int i = 0; i < 8; i++)
			children_[i].~Octree();
		free(children_);
		children_ = nullptr;
	}
	pool_freelist_->free(pool_index_);
	if (isRoot())
	{
		delete pool_freelist_;
		delete octree_pool_;
		delete split_mode_;
	}
	return;
}


void Octree::copy(const Octree& other)
{
	// NOTE: shallow copy
	layer_ = other.layer_;
	parent_ = other.parent_;
	children_ = other.children_;
	split_mode_ = other.split_mode_;

	pool_freelist_ = other.pool_freelist_;
	octree_pool_ = other.octree_pool_;
	pool_index_ = other.pool_index_;
	return;
}


void Octree::clear()
{
	if (isRoot())
	{
		for (unsigned int i = 0; i < 8; i++)
		{
			children_[i].clear();
		}
	}
	else
	{
		if (children_)
		{
			for (unsigned int i = 0; i < 8; i++)
				children_[i].~Octree();
			free(children_);
			children_ = nullptr;
		}
		setMaterialType(0);
		setUniformity(true);
	}
	return;
}


void Octree::setVoxel(int32_t x, int32_t y, int32_t z, uint16_t material_type)
{
	return setVoxelAtLayer(x, y, z, material_type, 0);
}


/* ---------------------------------------------------------------- *\
 * Set a voxel at a specific layer/precision. The octree is treated
 * as if <layer> was layer 0, and coordinates are adjusted
 * accordingly. Beware that this will remove any octree data at a
 * lower precision.
\* ---------------------------------------------------------------- */
void Octree::setVoxelAtLayer(int32_t x, int32_t y, int32_t z,
		uint16_t material_type, int layer)
{
	if (layer < 0)
	{
		throw std::runtime_error("Cannot set voxel at layer with negative value!");
	}

	if (isUniform() && material_type == getMaterialType())
	{
		// no change
		return;
	}
	if (layer_ == layer)
	{
		if (x != 0 || y != 0 || z != 0)
		{
			throw std::runtime_error("setVoxel() out of bounds of octree!");
		}
		clear();
		setMaterialType(material_type);
		if (parent_)
		{
			parent_->simpleUpdateLOD();
			parent_->simpleMerge(); // NOTE: this always has to come last since it
															// might destroy this octree object
		}
		return;
	}
	else
	{
		if (isUniform())
		{
			split();
		}
		int relative_layer = layer_ - layer;
		unsigned int quarter_axis_size;
		unsigned int adder;
		unsigned int subtracter;
		if (relative_layer == 1)
		{
			quarter_axis_size = 0;
			adder = 1;
			subtracter = 0;
		}
		else
		{
			quarter_axis_size = (1u << (relative_layer -2));
			adder = quarter_axis_size;
			subtracter = quarter_axis_size;
		}
		unsigned int child_index = 0;
		if (x < 0)
		{
			x += adder;
		}
		else
		{
			child_index |= 1u;
			x -= subtracter;
		}
		if (y < 0)
		{
			y += adder;
		}
		else
		{
			child_index |= 2u;
			y -= subtracter;
		}
		if (z < 0)
		{
			z += adder;
		}
		else
		{
			child_index |= 4u;
			z -= subtracter;
		}
		children_[child_index].setVoxelAtLayer(x, y, z, material_type, layer);
	}
	return;
}


VoxelTypeElement Octree::getVoxel(int32_t x, int32_t y, int32_t z)
{
	return getVoxelAtLayer(x, y, z, 0);
}


/* ---------------------------------------------------------------- *\
 * Similar to setVoxelAtLayer(), the octree is treated as if
 * <layer> was layer 0, and coordinates are adjusted accordingly.
 * In order for this to work properly, material_type_ must be set
 * properly at all layers of this octree.
\* ---------------------------------------------------------------- */
VoxelTypeElement Octree::getVoxelAtLayer(int32_t x, int32_t y, int32_t z, int layer)
{
	if (layer < 0)
	{
		throw std::runtime_error("Cannot get voxel at layer with negative value!");
	}

	if (isUniform() || layer_ == layer)
	{
		return getMaterialType();
	}
	else
	{
		int relative_layer = layer_ - layer;
		unsigned int quarter_axis_size;
		unsigned int adder;
		unsigned int subtracter;
		if (relative_layer == 1)
		{
			quarter_axis_size = 0;
			adder = 1;
			subtracter = 0;
		}
		else
		{
			quarter_axis_size = (1u << (relative_layer-2));
			adder = quarter_axis_size;
			subtracter = quarter_axis_size;
		}
		unsigned int child_index = 0;
		if (x < 0)
		{
			x += adder;
		}
		else
		{
			child_index |= 1u;
			x -= subtracter;
		}
		if (y < 0)
		{
			y += adder;
		}
		else
		{
			child_index |= 2u;
			y -= subtracter;
		}
		if (z < 0)
		{
			z += adder;
		}
		else
		{
			child_index |= 4u;
			z -= subtracter;
		}
		return children_[child_index].getVoxelAtLayer(x, y, z, layer);
	}
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
}


/* ---------------------------------------------------------------- *\
 * Update the approximated material type based on the immediate
 * children
\* ---------------------------------------------------------------- */
void Octree::simpleUpdateLOD()
{
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
}


VoxelTypeElement Octree::calculateMaterialTypeFromChildren()
{
	if (isUniform())
	{
		return getMaterialType();
	}
	// return the material type of the 0th child
	return children_[0].getMaterialType();
}


unsigned int Octree::prepareForDescent(int32_t *x, int32_t *y, int32_t *z)
{
	unsigned int child_index = 0;
	if (*x < 0)
	{
		*x += 1u << (layer_-1);
	}
	else
	{
		child_index |= 1u;
		*x -= (1u << (layer_-1)) - 1;
	}
	if (*y < 0)
	{
		*y += 1u << (layer_-1);
	}
	else
	{
		child_index |= 2u;
		*y -= (1u << (layer_-1)) - 1;
	}
	if (*z < 0)
	{
		*z += 1u << (layer_-1);
	}
	else
	{
		child_index |= 4u;
		*z -= (1u << (layer_-1)) - 1;
	}
	return child_index;
}


void Octree::mergeOctree(Octree *other, int32_t x, int32_t y, int32_t z
		)
{
	other->mergeIntoOctree(this, x, y, z);
	return;
}


void Octree::mergeIntoOctree(Octree *other, int32_t x, int32_t y, int32_t z)
{
	// TODO: temp!!
	other->octree_pool_->resize(octree_pool_->size());
	memcpy(other->octree_pool_->data(), octree_pool_->data(), octree_pool_->size()*sizeof(OctreeNode));
	return;

	if (isUniform() && getMaterialType() == 0)
	{
		return;
	}
	if (layer_ == 0)
	{
		other->setVoxel(x, y, z, getMaterialType());
	}
	else
	{
		int quarter_axis_size;
		int adder;
		int subtracter;
		int upper_end_adder;
		int lower_end_subtracter;
		if (layer_ == 1)
		{
			quarter_axis_size = 0;
			adder = 0;
			subtracter = 1;
			upper_end_adder = 0;
			lower_end_subtracter = 1;
		}
		else
		{
			quarter_axis_size = (1u << (layer_-2));
			adder = quarter_axis_size;
			subtracter = quarter_axis_size;
			upper_end_adder = (1u << (layer_-1))-1;
			lower_end_subtracter = (1u << (layer_-1));
		}
		if (isUniform())
		{
			// NOTE: experimental
			other->setVoxelTypeWithinBounds(getMaterialType(),
					x - lower_end_subtracter,
					y - lower_end_subtracter,
					z - lower_end_subtracter,
					x + upper_end_adder,
					y + upper_end_adder,
					z + upper_end_adder
					);
		}
		else
		{
			int child_x, child_y, child_z;
			for (unsigned int i = 0; i < 8; i++)
			{
				if (i & 1u)
					child_x = x + adder;
				else
					child_x = x - subtracter;
				if (i & 2u)
					child_y = y + adder;
				else
					child_y = y - subtracter;
				if (i & 4u)
					child_z = z + adder;
				else
					child_z = z - subtracter;
				children_[i].mergeIntoOctree(other, child_x, child_y, child_z);
			}
		}
	}
	return;
}


/* ---------------------------------------------------------------- *\
 * inclusive
\* ---------------------------------------------------------------- */
void Octree::setVoxelTypeWithinBounds(VoxelTypeElement voxel_type,
		int32_t x_min,
		int32_t y_min,
		int32_t z_min,
		int32_t x_max,
		int32_t y_max,
		int32_t z_max
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
	int32_t new_x_min = roundUpToInterval(x_min, octree_node_size);
	int32_t new_y_min = roundUpToInterval(y_min, octree_node_size);
	int32_t new_z_min = roundUpToInterval(z_min, octree_node_size);
	int32_t new_x_max = new_x_min + octree_node_size - 1;
	int32_t new_y_max = new_y_min + octree_node_size - 1;
	int32_t new_z_max = new_z_min + octree_node_size - 1;

	// check if this box matches the largest possible nearby octree node
	if (new_x_min == x_min && new_x_max == x_max &&
			new_y_min == y_min && new_y_max == y_max &&
			new_z_min == z_min && new_z_max == z_max)
	{
		// bit shifts may be implementation specific for negative numbers?
		//setVoxelAtLayer(new_x_min >> highest_layer, new_y_min >> highest_layer,
		//		new_z_min >> highest_layer, voxel_type, highest_layer);
		int32_t factor = 1;
		for (int i = 0; i < highest_layer; i++) factor *= 2;
		setVoxelAtLayer(new_x_min/factor, new_y_min/factor, new_z_min/factor,
				voxel_type, highest_layer);
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
	std::vector<int32_t> x_min_separators, x_max_separators;
	if (x_min != new_x_min)
	{
		x_min_separators.push_back(x_min);
		x_max_separators.push_back(new_x_min-1);
	}
	for (int i = 1; i*octree_node_size <= x_max - new_x_min + 1; i++)
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
	std::vector<int32_t> y_min_separators, y_max_separators;
	if (y_min != new_y_min)
	{
		y_min_separators.push_back(y_min);
		y_max_separators.push_back(new_y_min-1);
	}
	for (int i = 1; i*octree_node_size <= y_max - new_y_min + 1; i++)
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
	std::vector<int32_t> z_min_separators, z_max_separators;
	if (z_min != new_z_min)
	{
		z_min_separators.push_back(z_min);
		z_max_separators.push_back(new_z_min-1);
	}
	for (int i = 1; i*octree_node_size <= z_max - new_z_min + 1; i++)
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


int32_t Octree::roundUpToInterval(int32_t val, uint32_t interval)
{
	if (interval == 0)
		return val;

	int32_t remainder = abs(val) % interval;
	if (remainder == 0)
		return val;

	if (val < 0)
		return -(abs(val) - remainder);
	return val + (interval - remainder);
}


void Octree::split()
{
	if (!children_)
	{
		children_ = reinterpret_cast<Octree*>(malloc(8*sizeof(Octree)));
	}
	VoxelTypeElement voxel_type;
	switch (*split_mode_)
	{
		case SPLIT_MODE_NORMAL:
			voxel_type = getMaterialType();
			break;
		case SPLIT_MODE_AIRFILL:
			voxel_type = static_cast<VoxelTypeElement>(0);
			break;
		default:
			voxel_type = getMaterialType();
			break;
	}
	for (unsigned int i = 0; i < 8; i++)
	{
		new (&children_[i]) Octree(this, layer_-1, i);
		children_[i].setMaterialType(voxel_type);
	}
	setUniformity(false);
	return;
}

} // namespace Anthrax
