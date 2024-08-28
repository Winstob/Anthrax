/* ---------------------------------------------------------------- *\
 * input_handler.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-08-28
\* ---------------------------------------------------------------- */

#include "input_handler.hpp"

#include <iostream>

namespace Anthrax
{


// static member allocation
float InputHandler::mouse_x_;
float InputHandler::mouse_y_;


InputHandler::InputHandler(GLFWwindow *window)
{
	window_ = window;
	glfwSetCursorPosCallback(window_, cursorPosCallback);
}


void InputHandler::destroy()
{
}


void InputHandler::beginNewFrame()
{
	// frame time management
	float current_time = static_cast<float>(glfwGetTime());
	if (frame_start_time_ != 0.0)
	{
		frame_time_ = current_time - frame_start_time_;
	}
	frame_start_time_ = current_time;

	// record mouse movement
	mouse_x_difference_ = mouse_x_ - previous_mouse_x_;
	previous_mouse_x_ = mouse_x_;
	mouse_y_difference_ = mouse_y_ - previous_mouse_y_;
	previous_mouse_y_ = mouse_y_;

	glfwPollEvents();
	return;
}


// --------------------------------------------------
// GLFW callbacks
// --------------------------------------------------

void InputHandler::cursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
	mouse_x_ = static_cast<float>(xpos);
	mouse_y_ = static_cast<float>(ypos);
}


void InputHandler::scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
}


void InputHandler::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	/*
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window_, GL_TRUE);
	}
	*/
}

} // namespace Anthrax

