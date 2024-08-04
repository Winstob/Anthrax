/* ---------------------------------------------------------------- *\
 * anthrax.hpp
 * Author: Gavin Ralston
 * Date Created: 2023-12-10
\* ---------------------------------------------------------------- */
#ifndef ANTHRAX_HPP
#define ANTHRAX_HPP

#define PI 3.14159265

#include "vulkan_manager.hpp"

/*
#include <glad/glad.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
*/

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <vector>
#include <iostream>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H

//#include "shader.hpp"
#include "octree.hpp"
#include "camera.hpp"
#include "character.hpp"
#include "idmap.hpp"
#include "text.hpp"
#include "intfloat.hpp"

namespace Anthrax
{

class Anthrax
{
public:
	Anthrax();
	~Anthrax();
	int init();
	void setFOV(float angle);
	void exit();
	void renderFrame();
	bool windowShouldClose() const { return false; }//glfwWindowShouldClose(window_); }

	unsigned int addText(std::string text, float x, float y, float scale, glm::vec3 color);
	unsigned int addText(std::string text, float x, float y, float scale, float colorx, float colory, float colorz) { return addText(text, x, y, scale, glm::vec3(colorx, colory, colorz)); }
	int removeText(unsigned int id);

private:
	VulkanManager *vulkan_manager_;
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	void renderFullscreenQuad();
	void initializeShaders();
	void createWorld();
	void initializeWorldSSBOs();
	void updateCamera();
	void textTexturesSetup();
	void mainFramebufferSetup();
	void textFramebufferSetup();
	void renderText(std::string text, float x, float y, float scale, glm::vec3 color);
	void renderText(Text text) { renderText(text.text, text.x, text.y, text.scale, text.color); }

	// Shader passes
	/*
	Shader *main_pass_shader_;
	Shader *text_pass_shader_;
	Shader *screen_pass_shader_;
	*/

	static GLFWwindow* window_;
	VkInstance vulkan_instance_;

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
	unsigned int text_vao_ = 0, text_vbo_ = 0;

	Octree world_;
	Camera camera_;
	GLuint indirection_pool_ssbo_ = 0, voxel_type_pool_ssbo_ = 0, lod_pool_ssbo_ = 0;

	GLuint main_pass_framebuffer_ = 0, main_pass_texture_ = 0;
	GLuint text_pass_framebuffer_ = 0, text_pass_texture_ = 0;

	std::map<GLchar, Character> character_map_;
	IDMap<Text> texts_;

	Buffer ssbo_;

};

} // namespace Anthrax

#endif // ANTHRAX_HPP
