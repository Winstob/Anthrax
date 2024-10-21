/* ---------------------------------------------------------------- *\
 * material.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-10-20
\* ---------------------------------------------------------------- */

#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <glm/glm.hpp>

#include "tools.hpp"

namespace Anthrax
{

class Material
{
public:
	Material(float red, float green, float blue, float alpha=1.0);
	Material() : Material(0.0, 0.0, 0.0, 0.0) {}
	void copy(const Material &other);
	Material(const Material &other) { copy(other); }
	Material &operator=(const Material &other) { copy(other); return *this; }

	struct PackedMaterial
	{
		// Must match material struct in GPU code
		alignas(16) glm::vec4 color;
	};

	PackedMaterial pack();
private:
	float rgb_[3];
	float alpha_;
};

} // namespace Anthrax

#endif // MATERIAL_HPP
