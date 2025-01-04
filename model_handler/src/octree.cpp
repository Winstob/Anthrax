/* ---------------------------------------------------------------- *\
 * octree.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-12-12
\* ---------------------------------------------------------------- */

#include "octree.hpp"

#include <iostream>

namespace Anthrax
{

Octree::Octree(Octree *parent, int layer)
{
	parent_ = parent;
	layer_ = layer;
	children_ = nullptr;
	material_type_ = 0;
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
}


void Octree::copy(const Octree& other)
{
	// NOTE: shallow copy
	layer_ = other.layer_;
	parent_ = other.parent_;
	children_ = other.children_;
	material_type_ = other.material_type_;
	return;
}


void Octree::setVoxel(int32_t x, int32_t y, int32_t z, uint16_t material_type)
{
	if (isUniform() && material_type == material_type_)
	{
		// no change
		return;
	}
	if (layer_ == 0)
	{
		if (x != 0 || y != 0 || z != 0)
		{
			throw std::runtime_error("setVoxel() out of bounds of octree!");
		}
		material_type_ = material_type;
		parent_->simpleMerge();
		return;
	}
	else
	{
		if (!children_)
		{
			children_ = reinterpret_cast<Octree*>(malloc(8*sizeof(Octree)));
			for (unsigned int i = 0; i < 8; i++)
			{
				children_[i] = Octree(this, layer_-1);
				children_[i].setMaterialType(material_type_);
			}
		}
		unsigned int quarter_axis_size;
		unsigned int adder;
		unsigned int subtracter;
		if (layer_ == 1)
		{
			quarter_axis_size = 0;
			adder = 1;
			subtracter = 0;
		}
		else
		{
			quarter_axis_size = (1u << (layer_-2));
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
		children_[child_index].setVoxel(x, y, z, material_type);
	}
	return;
}


uint16_t Octree::getVoxel(int32_t x, int32_t y, int32_t z)
{
	if (isUniform())
	{
		return material_type_;
	}
	else
	{
		unsigned int quarter_axis_size;
		unsigned int adder;
		unsigned int subtracter;
		if (layer_ == 1)
		{
			quarter_axis_size = 0;
			adder = 1;
			subtracter = 0;
		}
		else
		{
			quarter_axis_size = (1u << (layer_-2));
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
		return children_[child_index].getVoxel(x, y, z);
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
	if (!children_)
	{
		return;
	}
	// check if children can be merged
	uint16_t material_type = children_[0].getMaterialType();
	bool can_merge_children = true;
	for (unsigned int i = 0; i < 8; i++)
	{
		if (!children_[i].isUniform() || children_[i].getMaterialType() != material_type)
		{
			can_merge_children = false;
			break;
		}
	}
	if (can_merge_children)
	{
		for (unsigned int i = 0; i < 8; i++)
		{
			children_[i].~Octree();
		}
		free(children_);
		children_ = nullptr;
		material_type_ = material_type;
		//std::cout << "Merged! Material type: " << material_type << " | Layer: " << layer_ << std::endl;
		if (parent_)
		{
			parent_->simpleMerge();
		}
	}
	return;
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
		world->setVoxel(x, y, z, material_type_);
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
		if (!children_)
		{
			for (unsigned int curr_x = x - lower_end_subtracter; curr_x <= x + upper_end_adder; curr_x++)
			{
				for (unsigned int curr_y = y - lower_end_subtracter; curr_y <= y + upper_end_adder; curr_y++)
				{
					for (unsigned int curr_z = z - lower_end_subtracter; curr_z <= z + upper_end_adder; curr_z++)
					{
						world->setVoxel(curr_x, curr_y, curr_z, material_type_);
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



	/*
	if (isUniform())
	{
		world->setIndirection(parent_world_indirection_pool_index_, this_child_index_, 0);
		world->setVoxelType(parent_world_indirection_pool_index_, this_child_index_, material_type_);
		world->setUniformity(parent_world_indirection_pool_index_, this_child_index_, 1);
	}
	else
	{
		uint32_t indirection_pool_index = world->mkAndReadIndirectionPool(parent_world_indirection_pool_index_, this_child_index_);
		//unsigned int child_index = prepareForDescent(&x, &y, &z);
		// the isUniform() check ensures that there are children, so no need to check again
		for (unsigned int i = 0; i < 8; i++)
		{
			children_[i].setParentWorldIndirectionPoolIndex(indirection_pool_index);
			children_[i].setChildIndex(i);
			addToWorld(world, x, y, z);
		}
	}
	return;
	*/
}

} // namespace Anthrax
