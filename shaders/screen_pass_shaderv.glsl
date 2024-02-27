/* ---------------------------------------------------------------- *\
 * screen_pass_shaderv.glsl
 * Author: Gavin Ralston
 * Date Created: 2024-02-26
\* ---------------------------------------------------------------- */
#version 460 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 tex_coords;

//out vec2 screen_position;
out vec2 texture_coords;

void main()
{
  gl_Position = vec4(position, 0.0, 1.0);
  texture_coords = tex_coords;
}
