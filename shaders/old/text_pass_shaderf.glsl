/* ---------------------------------------------------------------- *\
 * text_pass_shaderf.glsl
 * Author: Gavin Ralston
 * Date Created: 2024-02-26
\* ---------------------------------------------------------------- */
#version 460 core
layout (location = 0) out vec4 FragColor;
//out vec4 FragColor;

in vec2 texture_coords;

uniform sampler2D text_texture;
uniform vec3 color;

void main()
{
  FragColor = vec4(1.0, 1.0, 1.0, texture(text_texture, texture_coords).r);
  FragColor = vec4(color, 1.0) * FragColor;
}
