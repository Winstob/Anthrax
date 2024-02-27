/* ---------------------------------------------------------------- *\
 * screen_pass_shaderv.glsl
 * Author: Gavin Ralston
 * Date Created: 2024-02-26
\* ---------------------------------------------------------------- */
#version 460 core

layout (location = 0) in vec2 position;

out vec2 texture_coords;

void main()
{
  gl_Position = vec4(position, 0.0, 1.0);
  texture_coords = position*0.5 + 0.5;
}
