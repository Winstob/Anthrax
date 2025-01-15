/* ---------------------------------------------------------------- *\
 * octree.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-12-12
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
		indirection_pool_ = new std::vector<IndirectionElement>();
		voxel_type_pool_ = new std::vector<VoxelTypeElement>();
		uniformity_pool_ = new std::vector<UniformityElement>();
		split_mode_ = new SplitMode(SPLIT_MODE_NORMAL);
	}
	else
	{
		pool_freelist_ = parent_->pool_freelist_;
		indirection_pool_ = parent_->indirection_pool_;
		voxel_type_pool_ = parent_->voxel_type_pool_;
		uniformity_pool_ = parent_->uniformity_pool_;
		split_mode_ = parent_->split_mode_;
	}
	pool_index_ = pool_freelist_->alloc();
	if (isRoot() && pool_index_ != 0)
	{
		throw std::runtime_error("Pool index of Octree root node is not 0!");
	}
	// Ensure pools have sufficient sizes
	if (indirection_pool_->size() <= pool_index_*8)
	{
		indirection_pool_->resize((pool_index_+1)*8, 0);
	}
	if (voxel_type_pool_->size() <= pool_index_)
	{
		voxel_type_pool_->resize(pool_index_+1, 0);
	}
	if (uniformity_pool_->size() <= pool_index_/(8*sizeof(UniformityElement)))
	{
		uniformity_pool_->resize(pool_index_/(8*sizeof(UniformityElement))+1, 0);
	}

	if (!isRoot())
	{
		parent_->setIndirection(this_child_index_, pool_index_);
	}

	setUniformity(true);
	setMaterialType(0);
	children_ = nullptr;
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
		delete indirection_pool_;
		delete voxel_type_pool_;
		delete uniformity_pool_;
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
	indirection_pool_ = other.indirection_pool_;
	voxel_type_pool_ = other.voxel_type_pool_;
	uniformity_pool_ = other.uniformity_pool_;
	pool_index_ = other.pool_index_;
	return;
}


void Octree::clear()
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
	return;
}


void Octree::setVoxel(int32_t x, int32_t y, int32_t z, uint16_t material_type)
{
	return setVoxelAtLayer(x, y, z, material_type, 0);
}


/* ---------------------------------------------------------------- *\
 * Set a voxel at a specific layer/precision. The octree is treated
 * as if it were only <layer> layers deep, and coordinates are
 * adjusted accordingly. Beware that this will remove any octree
 * data at a lower precision.
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


uint16_t Octree::getVoxel(int32_t x, int32_t y, int32_t z)
{
	return getVoxelAtLayer(x, y, z, 0);
}


/* ---------------------------------------------------------------- *\
 * Similar to setVoxelAtLayer(), the octree is treated as if it
 * were only <layer> layers deep, and coordinates are adjusted
 * accordingly. In order for this to work properly, material_type_
 * must be set properly at all layers of this octree.
\* ---------------------------------------------------------------- */
uint16_t Octree::getVoxelAtLayer(int32_t x, int32_t y, int32_t z, int layer)
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
	uint16_t new_material_type = calculateMaterialTypeFromChildren();
	bool material_type_changed = (new_material_type != getMaterialType());
	setMaterialType(new_material_type);
	if (material_type_changed && parent_)
	{
		parent_->simpleUpdateLOD();
	}
	return;
}


uint16_t Octree::calculateMaterialTypeFromChildren()
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


void Octree::addToWorld(World *world, unsigned int x, unsigned int y, unsigned int z)
{
	// TODO: do this a better way
	if (layer_ == 0)
	{
		world->setVoxel(x, y, z, getMaterialType());
	}
	else
	{
		unsigned int quarter_axis_size;
		unsigned int adder;
		unsigned int subtracter;
		unsigned int upper_end_adder;
		unsigned int lower_end_subtracter;
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
			for (unsigned int curr_x = x - lower_end_subtracter; curr_x <= x + upper_end_adder; curr_x++)
			{
				for (unsigned int curr_y = y - lower_end_subtracter; curr_y <= y + upper_end_adder; curr_y++)
				{
					for (unsigned int curr_z = z - lower_end_subtracter; curr_z <= z + upper_end_adder; curr_z++)
					{
						world->setVoxel(curr_x, curr_y, curr_z, getMaterialType());
					}
				}
			}
		}
		else
		{
			unsigned int child_x, child_y, child_z;
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
				children_[i].addToWorld(world, child_x, child_y, child_z);
			}
		}
	}
	return;
}


void Octree::split()
{
	setUniformity(false);
	children_ = reinterpret_cast<Octree*>(malloc(8*sizeof(Octree)));
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
	return;
}

} // namespace Anthrax
