/* ---------------------------------------------------------------- *\
 * text_pass_shaderv.glsl
 * Author: Gavin Ralston
 * Date Created: 2024-02-26
\* ---------------------------------------------------------------- */
#version 460 core
layout (location = 0) in vec4 vertex;

out vec2 texture_coords;

uniform mat4 projection;

void main()
{
  gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
  texture_coords = vertex.zw;
}
