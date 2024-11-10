/* ---------------------------------------------------------------- *\
 * camera.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-02-06
\* ---------------------------------------------------------------- */
#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtx/quaternion.hpp"
#include <glm/gtx/vector_angle.hpp>

#include "intfloat.hpp"

namespace Anthrax
{

class Camera
{
public:
	Camera() : Camera(glm::ivec3(0)) {}
	Camera(glm::ivec3 in_position) : rotation(glm::quat(0.0, 0.0, 0.0, 0.0)) { position[0] = in_position.x; position[1] = in_position.y; position[2] = in_position.z; }
	Camera& operator=(const Camera &other)
	{
		position[0] = other.position[0];
		position[1] = other.position[1];
		position[2] = other.position[2];
		rotation = other.rotation;
		return *this;
	}
	glm::vec3 getUpDirection() { return glm::vec3(0.0, 1.0, 0.0); }
	/*
	glm::vec3 getUpDirection()
	{
		if (glm::dot(getUpLookDirection(), glm::vec3(0.0, 1.0, 0.0)) > 0.0)
			return glm::vec3(0.0, 1.0, 0.0);
		return glm::vec3(0.0, -1.0, 0.0);
	}
	*/
	glm::vec3 getUpLookDirection() { return (rotation * glm::vec3(0.0, 1.0, 0.0)); }
	glm::vec3 getRightLookDirection() { return (rotation * glm::vec3(1.0, 0.0, 0.0)); }
	glm::vec3 getForwardLookDirection() { return (rotation * glm::vec3(0.0, 0.0, -1.0)); }

	Intfloat position[3];
	glm::quat rotation;
	float fov;
	float focal_distance = 1.0;
private:
};

} // namespace Anthrax
#endif // CAMERA_HPP
