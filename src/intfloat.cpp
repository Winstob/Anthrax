/* ---------------------------------------------------------------- *\
 * intfloat.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-03-04
\* ---------------------------------------------------------------- */

#include "intfloat.hpp"

namespace Anthrax
{
} // namespace Anthrax


glm::ivec3 iComponents3(Anthrax::Intfloat *vec3)
{
	return glm::vec3(vec3[0].int_component, vec3[1].int_component, vec3[2].int_component);
}

glm::vec3 fComponents3(Anthrax::Intfloat *vec3)
{
	return glm::vec3(vec3[0].dec_component, vec3[1].dec_component, vec3[2].dec_component);
}

