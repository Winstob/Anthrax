/* ---------------------------------------------------------------- *\
 * anthrax.hpp
 * Author: Gavin Ralston
 * Date Created: 2023-12-10
\* ---------------------------------------------------------------- */
#ifndef ANTHRAX_HPP
#define ANTHRAX_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <iostream>

#include "shader.hpp"

namespace Anthrax
{

class Anthrax
{
public:
  Anthrax();
  ~Anthrax();
  int initWindow();
  void renderFrame();
private:
  void renderFullscreenQuad();
  void initializeShaders();

  // Shader passes
  Shader *main_pass_shader_;

  static GLFWwindow* window_;
  // Settings
  static unsigned int window_width_;
  static unsigned int window_height_;
  // Callbacks
  static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
  static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
  static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
  static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

  unsigned int quad_vao_ = 0, quad_vbo_ = 0;
};

} // namespace Anthrax

#endif // ANTHRAX_HPP
