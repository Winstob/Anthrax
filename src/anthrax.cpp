/* ---------------------------------------------------------------- *\
 * anthrax.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-02-03
\* ---------------------------------------------------------------- */
#include "anthrax.hpp"

#ifndef WINDOW_NAME
#define WINDOW_NAME Anthrax
#endif
#define xstr(s) str(s)
#define str(s) #s


namespace Anthrax
{

// Static member allocation
GLFWwindow *Anthrax::window_;
unsigned int Anthrax::window_width_ = 0;
unsigned int Anthrax::window_height_ = 0;
bool Anthrax::window_size_changed_ = false;


Anthrax::Anthrax()
{
  window_width_ = 600;
  window_height_ = 400;
  world_ = Octree(8);
}


Anthrax::~Anthrax()
{
  exit();
}


int Anthrax::initWindow()
{
  // Initialize glfw
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // Create glfw window
  window_ = glfwCreateWindow(window_width_, window_height_, xstr(WINDOW_NAME), NULL, NULL);
  //window = glfwCreateWindow(window_width_, window_height_, x_str(WINDOW_NAME), glfwGetPrimaryMonitor(), NULL);
  if (window_ == NULL)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window_);
  glfwSetFramebufferSizeCallback(window_, framebufferSizeCallback);
  glfwSetCursorPosCallback(window_, cursorPosCallback);
  glfwSetScrollCallback(window_, scrollCallback);
  glfwSetKeyCallback(window_, keyCallback);

  // tell GLFW to capture our mouse
  glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // glad: load all OpenGL function pointers
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // Face culling
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  // Disable vsync
  glfwSwapInterval(0);

  initializeShaders();

  createWorld();
  initializeWorldSSBOs();

  /*
  int tmp;
  glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &tmp);
  std::cout << (tmp >> 20) << std::endl;
  */

  return 0;
}


void Anthrax::exit()
{
  delete main_pass_shader_;
  glDeleteBuffers(1, &indirection_pool_ssbo_);
  glDeleteBuffers(1, &lod_pool_ssbo_);
  glfwTerminate();
}


void Anthrax::renderFrame()
{
  if (window_size_changed_)
  {
    window_size_changed_ = false;
  }
  glClear(GL_COLOR_BUFFER_BIT);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  main_pass_shader_->use();
  renderFullscreenQuad();

  glfwSwapBuffers(window_);
  glfwPollEvents();
  return;
}


void Anthrax::renderFullscreenQuad()
{
  if (quad_vao_ == 0)
  {
    // Set up fullscreen quad
    float quad_vertices_[] = {
    // positions  // texture coordinates
    -1.0, -1.0,   0.0, 0.0,
    1.0, -1.0,    1.0, 0.0,
    -1.0, 1.0,    0.0, 1.0,
    1.0, 1.0,     1.0, 1.0
    };
    glGenVertexArrays(1, &quad_vao_);
    glGenBuffers(1, &quad_vbo_);
    glBindVertexArray(quad_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices_), &quad_vertices_, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
  }
  glBindVertexArray(quad_vao_);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
  return;
}


void Anthrax::initializeShaders()
{
  std::string shader_directory = xstr(SHADER_DIRECTORY);
  shader_directory += "/";

  main_pass_shader_ = new Shader(Shader::ShaderInputType::FILEPATH, (shader_directory + "main_pass_shaderv.glsl").c_str(), (shader_directory + "main_pass_shaderf.glsl").c_str());
  return;
}


void Anthrax::createWorld()
{
  for (unsigned int i = 0; i < world_.num_indices_; i++)
  {
    for (unsigned int j = 0; j < 8; j++)
    {
      int k = i*8 + j;
      if (j == 0 || j == 3 || j == 5 || j == 6)
      {
        world_.indirection_pool_[k] = 0x0000;
      }
      else
      {
        world_.indirection_pool_[k] = k + 1;
      }
    }
    world_.voxel_type_pool_[i] = 1;
  }
  return;
}


void Anthrax::initializeWorldSSBOs()
{
  glGenBuffers(1, &indirection_pool_ssbo_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, indirection_pool_ssbo_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, 8 * sizeof(uint32_t) * world_.num_indices_, world_.indirection_pool_, GL_DYNAMIC_STORAGE_BIT);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, indirection_pool_ssbo_);

  glGenBuffers(1, &voxel_type_pool_ssbo_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxel_type_pool_ssbo_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * world_.num_indices_, world_.voxel_type_pool_, GL_DYNAMIC_STORAGE_BIT);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, voxel_type_pool_ssbo_);

  glGenBuffers(1, &lod_pool_ssbo_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, lod_pool_ssbo_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * world_.num_indices_, world_.lod_pool_, GL_DYNAMIC_STORAGE_BIT);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, lod_pool_ssbo_);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  return;
}

// ----------------------------------------------------------------
// Callbacks
// ----------------------------------------------------------------

void Anthrax::framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
  glViewport(0, 0, width, height);
  window_width_ = width;
  window_height_ = height;
  window_size_changed_ = true;
}


void Anthrax::cursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
}


void Anthrax::scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
}


void Anthrax::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window_, GL_TRUE);
  }
}

} // namespace Anthrax
