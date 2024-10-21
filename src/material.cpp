/* ---------------------------------------------------------------- *\
 * material.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-10-20
\* ---------------------------------------------------------------- */
#include "material.hpp"

namespace Anthrax
{

Material::Material(float red, float green, float blue, float alpha)
{
	rgb_[0] = red;
	rgb_[1] = green;
	rgb_[2] = blue;
	alpha_ = alpha;
}

void Material::copy(const Material &other)
{
	rgb_[0] = other.rgb_[0];
	rgb_[1] = other.rgb_[1];
	rgb_[2] = other.rgb_[2];
	alpha_ = other.alpha_;
	return;
}

Material::PackedMaterial Material::pack()
{
	PackedMaterial packed_material;
	packed_material.color.r = rgb_[0];
	packed_material.color.g = rgb_[1];
	packed_material.color.b = rgb_[2];
	packed_material.color.a = alpha_;

	return packed_material;
}

} // namespace Anthrax
