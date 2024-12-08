/* ---------------------------------------------------------------- *\
 * input_handler.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-08-28
\* ---------------------------------------------------------------- */

#ifndef INPUT_HANDLER_HPP
#define INPUT_HANDLER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include <math.h>


namespace Anthrax
{


class InputHandler
{
public:
	InputHandler(GLFWwindow *window);
	InputHandler() {};
	void destroy();

	void beginNewFrame();
	float getFrameTime() { return frame_time_; }
	float getMouseMovementX() { return mouse_x_difference_; }
	float getMouseMovementY() { return mouse_y_difference_; }

	GLFWAPI int getKey(int key) { return glfwGetKey(window_, key); }

	void captureMouse() { glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED); }
	void releaseMouse() { glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL); }

private:
	GLFWwindow *window_;
	static float mouse_x_;
	static float mouse_y_;
	float previous_mouse_x_;
	float previous_mouse_y_;
	float mouse_x_difference_;
	float mouse_y_difference_;


	float frame_start_time_ = 0.0;
	float frame_time_;

	static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);
	static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
	static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
};


} // namespace Anthrax

#endif // INPUT_HANDLER_HPP
