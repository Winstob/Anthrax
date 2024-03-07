/* ---------------------------------------------------------------- *\
 * anthrax.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-02-03
\* ---------------------------------------------------------------- */
#include "anthrax.hpp"

#ifndef WINDOW_NAME
#define WINDOW_NAME Anthrax
#endif

#ifndef FONT_DIRECTORY
#define FONT_DIRECTORY fonts
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
  window_width_ = 800;
  window_height_ = 600;
  int world_size = 24;
  world_ = Octree(world_size);
  camera_ = Camera(glm::vec3(pow(2, world_size-3), pow(2, world_size-3), 0.0));
  //camera_ = Camera(glm::vec3(0.0, 0.0, 0.0));
  //camera_ = Camera(glm::ivec3(0, 0, 0));
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
  /*
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  */

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Disable vsync
  glfwSwapInterval(0);

  initializeShaders();
  textTexturesSetup();
  mainFramebufferSetup();
  textFramebufferSetup();

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


void Anthrax::setFOV(float angle)
{
  // Screen coordinates are between -1.0 and 1.0
  // Calculate the focal distance given a fov angle in degrees
  camera_.fov = angle;
  camera_.focal_distance = 1.0/tan(angle*0.5*PI/180.0);
  return;
}


void Anthrax::exit()
{
  delete main_pass_shader_;
  delete text_pass_shader_;
  delete screen_pass_shader_;
  glDeleteFramebuffers(1, &main_pass_framebuffer_);
  glDeleteTextures(1, &main_pass_texture_);
  glDeleteFramebuffers(1, &text_pass_framebuffer_);
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
    mainFramebufferSetup();
    textFramebufferSetup();
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

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  // Main pass
  glBindFramebuffer(GL_FRAMEBUFFER, main_pass_framebuffer_);
  glClear(GL_COLOR_BUFFER_BIT);
  main_pass_shader_->use();
  main_pass_shader_->setInt("octree_layers", world_.num_layers_);
  main_pass_shader_->setFloat("focal_distance", camera_.focal_distance);
  main_pass_shader_->setInt("screen_width", window_width_);
  main_pass_shader_->setInt("screen_height", window_height_);
  main_pass_shader_->setIvec3("camera_position.int_component", iComponents3(camera_.position));
  main_pass_shader_->setVec3("camera_position.dec_component", fComponents3(camera_.position));
  main_pass_shader_->setVec3("camera_right", camera_.getRightLookDirection());
  main_pass_shader_->setVec3("camera_up", camera_.getUpLookDirection());
  main_pass_shader_->setVec3("camera_forward", camera_.getForwardLookDirection());
  //main_pass_shader_->setVec3("sunlight.direction", glm::normalize(glm::vec3(0.4, -0.5, 0.1)));
  main_pass_shader_->setVec3("sunlight.direction", glm::vec3(glm::cos(glfwGetTime()/16), -glm::sin(glfwGetTime()/16), glfwGetTime()/16));
  main_pass_shader_->setVec3("sunlight.scatter_color", glm::normalize(glm::vec3(0.8, 0.8, 1.0)));
  main_pass_shader_->setVec3("sunlight.color", glm::normalize(glm::vec3(0.2, 0.2, 0.0)));
  renderFullscreenQuad();

  // Text pass
  glBindFramebuffer(GL_FRAMEBUFFER, text_pass_framebuffer_);
  glClear(GL_COLOR_BUFFER_BIT);
  text_pass_shader_->use();
  //renderText("Hi Bros", 25.0, static_cast<float>(window_height_)-45.0, 0.5, glm::vec3(1.0, 1.0, 1.0));
  for (unsigned int i = 0; i < texts_.size(); i++)
  {
    if (texts_.exists(i))
      renderText(texts_[i]);
  }

  // Screen pass
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  screen_pass_shader_->use();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, main_pass_texture_);
  renderFullscreenQuad();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, text_pass_texture_);
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

  text_pass_shader_ = new Shader(Shader::ShaderInputType::FILEPATH, (shader_directory + "text_pass_shaderv.glsl").c_str(), (shader_directory + "text_pass_shaderf.glsl").c_str());

  screen_pass_shader_ = new Shader(Shader::ShaderInputType::FILEPATH, (shader_directory + "screen_pass_shaderv.glsl").c_str(), (shader_directory + "screen_pass_shaderf.glsl").c_str());

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
    world_.lod_pool_[i] = 0x000000FF; // White, opaque
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
  float walking_speed = 1.5; // meters per second
  float speed_multiplier = walking_speed * 100.0f; // Each voxel is 1cm in width
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

  /*
  camera_.position_dec_component += motion_direction;
  camera_.position_int_component += glm::ivec3(camera_.position_dec_component);
  camera_.position_dec_component -= glm::ivec3(camera_.position_dec_component);
  */
  camera_.position[0] += motion_direction.x;
  camera_.position[1] += motion_direction.y;
  camera_.position[2] += motion_direction.z;
  /*
  std::cout << camera_.position[0].int_component << std::endl;
  std::cout << camera_.position[0].dec_component << std::endl << std::endl;

  int int_component = camera_.position[0].int_component + (0x1 << (world_.num_layers_-2));
  float dec_component = camera_.position[0].dec_component;
  if (dec_component < 0.0)
  {
    dec_component += 1.0;
    int_component -= 1;
  }
  std::cout << int_component << std::endl;
  std::cout << dec_component << std::endl << std::endl << std::endl;
  */
  return;
}


void Anthrax::mainFramebufferSetup()
{
  if (main_pass_framebuffer_ == 0)
  {
    glGenFramebuffers(1, &main_pass_framebuffer_);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, main_pass_framebuffer_);

  if (main_pass_texture_ == 0)
  {
    glGenTextures(1, &main_pass_texture_);
  }

  glBindTexture(GL_TEXTURE_2D, main_pass_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_width_, window_height_, 0, GL_RGBA, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, main_pass_texture_, 0);

  unsigned int attachment = GL_COLOR_ATTACHMENT0;
  glDrawBuffers(1, &attachment);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "Framebuffer not complete!" << std::endl;

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return;
}


void Anthrax::textTexturesSetup()
{
  FT_Library ft;
  if (FT_Init_FreeType(&ft))
  {
    std::cout << "ERROR::FREETYPE: Could not init FreeType library" << std::endl;
    return;
  }
  FT_Face face;
  std::string font_directory = xstr(FONT_DIRECTORY);
  //if (FT_New_Face(ft, (font_directory + "/Ubuntu-BI.ttf").c_str(), 0, &face))
  //if (FT_New_Face(ft, (font_directory + "/tmp.ttf").c_str(), 0, &face))
  if (FT_New_Face(ft, "fonts/UbuntuMono-R.ttf", 0, &face))
  {
    std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    return;
  }
  FT_Set_Pixel_Sizes(face, 0, 64);
  glPixelStoref(GL_UNPACK_ALIGNMENT, 1);

  for (unsigned char c = 0; c < 128; c++)
  {
    // Load character glyph
    if (FT_Load_Char(face, c, FT_LOAD_RENDER))
    {
      std::cout << "ERROR::FREETYPE: Failed to load glyph" << std::endl;
      continue;
    }
    // Generate texture
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        face->glyph->bitmap.width,
        face->glyph->bitmap.rows,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        face->glyph->bitmap.buffer
        );
    // Set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Store character info
    Character character = Character(
      texture,
      glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
      glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
      static_cast<unsigned int>(face->glyph->advance.x)
      );
    character_map_.insert(std::pair<GLchar, Character>(c, character));
  }
  glBindTexture(GL_TEXTURE_2D, 0);
  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  if (text_vao_ == 0)
  {
    glGenVertexArrays(1, &text_vao_);
    glGenBuffers(1, &text_vbo_);
    glBindVertexArray(text_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*6*4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4*sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  return;
}

void Anthrax::textFramebufferSetup()
{

  if (text_pass_framebuffer_ == 0)
  {
    glGenFramebuffers(1, &text_pass_framebuffer_);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, text_pass_framebuffer_);

  if (text_pass_texture_ == 0)
  {
    glGenTextures(1, &text_pass_texture_);
  }

  glBindTexture(GL_TEXTURE_2D, text_pass_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_width_, window_height_, 0, GL_RGBA, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, text_pass_texture_, 0);

  unsigned int attachment = GL_COLOR_ATTACHMENT0;
  glDrawBuffers(1, &attachment);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "Framebuffer not complete!" << std::endl;

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return;
}


void Anthrax::renderText(std::string text, float x, float y, float scale, glm::vec3 color)
{
  text_pass_shader_->use();
  text_pass_shader_->setVec3("color", color);
  glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(window_width_), 0.0f, static_cast<float>(window_height_));
  text_pass_shader_->setMat4("projection", projection);
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(text_vao_);

  // Iterate through all characters
  std::string::const_iterator c;
  for (c = text.begin(); c != text.end(); c++) 
  {
    Character ch = character_map_[*c];

    float xpos = x + ch.Bearing.x * scale;
    float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

    float w = ch.Size.x * scale;
    float h = ch.Size.y * scale;
    // Update VBO for each character
    float vertices[6][4] = {
        { xpos,     ypos + h,   0.0f, 0.0f },            
        { xpos,     ypos,       0.0f, 1.0f },
        { xpos + w, ypos,       1.0f, 1.0f },

        { xpos,     ypos + h,   0.0f, 0.0f },
        { xpos + w, ypos,       1.0f, 1.0f },
        { xpos + w, ypos + h,   1.0f, 0.0f }           
    };
    // Render glyph texture over quad
    glBindTexture(GL_TEXTURE_2D, ch.TextureID);
    // Update content of VBO memory
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Render quad
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
    x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
  }
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);

  return;
}


unsigned int Anthrax::addText(std::string text, float x, float y, float scale, glm::vec3 color)
{
  return texts_.add(Text(text, x, y, scale, color));
}


int Anthrax::removeText(unsigned int id)
{
  return texts_.remove(id);
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
