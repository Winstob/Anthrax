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

namespace Anthrax
{

class Camera
{
public:
  Camera() : position(glm::vec3(0.0, 0.0, 0.0)), rotation(glm::quat(0.0, 0.0, 0.0, 0.0)) {}
  glm::vec3 getUpDirection() { return glm::vec3(0.0, 1.0, 0.0); }
  glm::vec3 getUpLookDirection() { return (rotation * glm::vec3(0.0, 1.0, 0.0)); }
  glm::vec3 getRightLookDirection() { return (rotation * glm::vec3(1.0, 0.0, 0.0)); }
  glm::vec3 getForwardLookDirection() { return (rotation * glm::vec3(0.0, 0.0, -1.0)); }

  glm::vec3 position;
  glm::quat rotation;
private:
};

} // namespace Anthrax
#endif // CAMERA_HPP
