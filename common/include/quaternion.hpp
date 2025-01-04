/* ---------------------------------------------------------------- *\
 * quaternion.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-12-17
\* ---------------------------------------------------------------- */
#ifndef QUATERNION_HPP
#define QUATERNION_HPP

#include <cstddef>
#include <stdexcept>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "tools.hpp"

namespace Anthrax
{

class Quaternion
{
public:
	Quaternion(float x, float y, float z, float w);
	Quaternion(float yaw, float pitch, float roll);
	Quaternion() : Quaternion(0.0, 0.0, 0.0, 1.0) {};
	Quaternion &operator=(const Quaternion &other) { copy(other); return *this; }
	Quaternion(const Quaternion &other) { copy(other); }
	void copy(const Quaternion &other);
	float operator[](size_t i);
	void normalize();

	std::vector<float> eulerAngles();

private:
	float x_, y_, z_, w_;
};

} // namespace Anthrax

#endif // QUATERNION_HPP
