/* ---------------------------------------------------------------- *\
 * anthrax.hpp
 * Author: Gavin Ralston
 * Date Created: 2023-12-10
\* ---------------------------------------------------------------- */
#ifndef ANTHRAX_HPP
#define ANTHRAX_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <iostream>

#include "shader.hpp"
#include "octree.hpp"
#include "camera.hpp"

namespace Anthrax
{

class Anthrax
{
public:
  Anthrax();
  ~Anthrax();
  int initWindow();
  void exit();
  void renderFrame();
  bool windowShouldClose() const { return glfwWindowShouldClose(window_); }
private:
  void renderFullscreenQuad();
  void initializeShaders();
  void createWorld();
  void initializeWorldSSBOs();
  void updateCamera();

  // Shader passes
  Shader *main_pass_shader_;

  static GLFWwindow* window_;

  float previous_frame_time_ = 0.0;
  float frame_time_difference_ = 0.0;
  static float mouse_x_;
  static float mouse_y_;
  float previous_mouse_x_;
  float previous_mouse_y_;
  float mouse_x_difference_;
  float mouse_y_difference_;
  // Settings
  static unsigned int window_width_;
  static unsigned int window_height_;
  static bool window_size_changed_;
  // Callbacks
  static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
  static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
  static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
  static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

  unsigned int quad_vao_ = 0, quad_vbo_ = 0;

  Octree world_;
  Camera camera_;
  GLuint indirection_pool_ssbo_ = 0, voxel_type_pool_ssbo_ = 0, lod_pool_ssbo_ = 0;

};

} // namespace Anthrax

#endif // ANTHRAX_HPP