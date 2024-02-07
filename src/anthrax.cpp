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
float Anthrax::mouse_x_;
float Anthrax::mouse_y_;
unsigned int Anthrax::window_width_ = 0;
unsigned int Anthrax::window_height_ = 0;
bool Anthrax::window_size_changed_ = false;


Anthrax::Anthrax()
{
  window_width_ = 600;
  window_height_ = 400;
  world_ = Octree(16);
  camera_ = Camera(glm::vec3(8192.0, 8192.0, 0.0));
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
  main_pass_shader_->use();
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
  glDeleteBuffers(1, &voxel_type_pool_ssbo_);
  glDeleteBuffers(1, &lod_pool_ssbo_);
  glfwTerminate();
}


void Anthrax::renderFrame()
{
  if (window_size_changed_)
  {
    window_size_changed_ = false;
  }

  float current_time = static_cast<float>(glfwGetTime());
  if (previous_frame_time_ != 0.0)
  {
    frame_time_difference_ = current_time - previous_frame_time_;
  }
  previous_frame_time_ = current_time;
  mouse_x_difference_ = mouse_x_ - previous_mouse_x_;
  previous_mouse_x_ = mouse_x_;
  mouse_y_difference_ = mouse_y_ - previous_mouse_y_;
  previous_mouse_y_ = mouse_y_;
  updateCamera();

  glClear(GL_COLOR_BUFFER_BIT);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  main_pass_shader_->use();
  main_pass_shader_->setInt("octree_layers", world_.num_layers_);
  main_pass_shader_->setFloat("focal_distance", 1.0);
  main_pass_shader_->setInt("screen_width", window_width_);
  main_pass_shader_->setInt("screen_height", window_height_);
  main_pass_shader_->setVec3("camera_position", camera_.position);
  main_pass_shader_->setVec3("camera_right", camera_.getRightLookDirection());
  main_pass_shader_->setVec3("camera_up", camera_.getUpLookDirection());
  main_pass_shader_->setVec3("camera_forward", camera_.getForwardLookDirection());
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
  unsigned int next_free_index = 1;
  for (unsigned int i = 0; i < world_.num_indices_; i++)
  {
    if (i >= (1 << 30)) break;
    for (unsigned int j = 0; j < 8; j++)
    {
      int k = i*8 + j;
      if (j == 0 || j == 3 || j == 5 || j == 6)
      {
        //world_.indirection_pool_[k] = next_free_index++;
        world_.indirection_pool_[k] = 1;
      }
      else
      {
        world_.indirection_pool_[k] = 0;
      }
    }
    if (i > 21844)
      world_.voxel_type_pool_[i] = 1;
    else
      world_.voxel_type_pool_[i] = 0;
  }
  return;
}


void Anthrax::initializeWorldSSBOs()
{
  glGenBuffers(1, &indirection_pool_ssbo_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, indirection_pool_ssbo_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, 8 * sizeof(uint32_t) * world_.num_indices_, world_.indirection_pool_, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, indirection_pool_ssbo_);

  glGenBuffers(1, &voxel_type_pool_ssbo_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxel_type_pool_ssbo_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * world_.num_indices_, world_.voxel_type_pool_, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, voxel_type_pool_ssbo_);

  glGenBuffers(1, &lod_pool_ssbo_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, lod_pool_ssbo_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * world_.num_indices_, world_.lod_pool_, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, lod_pool_ssbo_);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  return;
}


void Anthrax::updateCamera()
{
  // Update rotation
  float sensitivity = 0.01;
  glm::quat yaw_quaternion = glm::angleAxis(sensitivity*(-mouse_x_difference_), camera_.getUpDirection());
  glm::quat pitch_quaternion = glm::angleAxis(sensitivity*(-mouse_y_difference_), camera_.getRightLookDirection());
  camera_.rotation = glm::normalize(yaw_quaternion * pitch_quaternion * camera_.rotation);

  // Update position
  glm::vec3 motion_direction = glm::vec3(0.0, 0.0, 0.0);
  int speed_multiplier = 64;
  float motion_multiplier = frame_time_difference_ * speed_multiplier;

  if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS)
  {
    motion_direction.x += -1.0;
  }
  if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS)
  {
    motion_direction.x += 1.0;
  }
  if (glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
  {
    motion_direction.y += -1.0;
  }
  if (glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS)
  {
    motion_direction.y += 1.0;
  }
  if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS)
  {
    motion_direction.z += -1.0;
  }
  if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS)
  {
    motion_direction.z += 1.0;
  }

  glm::normalize(motion_direction);
  motion_direction *= motion_multiplier;
  motion_direction = camera_.rotation * motion_direction;
  camera_.position += motion_direction;
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
  mouse_x_ = static_cast<float>(xpos);
  mouse_y_ = static_cast<float>(ypos);
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
