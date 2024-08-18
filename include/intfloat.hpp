/* ---------------------------------------------------------------- *\
 * intfloat.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-03-04
\* ---------------------------------------------------------------- */

#ifndef INTFLOAT_HPP
#define INTFLOAT_HPP

#include <cstdint>
#include <math.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>


namespace Anthrax
{

class Intfloat
{
public:
	struct vec3
	{
		alignas(16) glm::ivec3 int_component;
		alignas(16) glm::vec3 dec_component;
	};

	Intfloat() : Intfloat(0, 0.0f) {}
	Intfloat(int32_t _int_component) : Intfloat(_int_component, 0.0) {}
	Intfloat(int32_t _int_component, float _dec_component) : int_component(_int_component), dec_component(_dec_component) {}

	Intfloat& operator=(const Intfloat &other)
	{
		int_component = other.int_component;
		dec_component = other.dec_component;
		return *this;
	}
	Intfloat& operator=(int &other)
	{
		int_component = other;
		dec_component = 0.0;
		return *this;
	}
	Intfloat& operator+(float add_val)
	{
		dec_component += add_val;
		int excess = floor(dec_component);
		dec_component -= excess;
		int_component += excess;
		if (int_component > 0)
		{
			if (dec_component < 0.0)
			{
				dec_component += 1.0;
				int_component -= 1;
			}
		}
		if (int_component < 0)
		{
			if (dec_component > 0.0)
			{
				dec_component -= 1.0;
				int_component += 1;
			}
		}
		return *this;
	}
	Intfloat& operator+=(float add_val)
	{
		return *this + add_val;
	}
	Intfloat& operator-(float sub_value) { return operator+(-sub_value); }

	int32_t int_component;
	float dec_component;
private:
};

} // namespace Anthrax

glm::ivec3 iComponents3(Anthrax::Intfloat *vec3);
glm::vec3 fComponents3(Anthrax::Intfloat *vec3);

#endif // INTFLOAT_HPP

