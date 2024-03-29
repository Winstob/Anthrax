/* ---------------------------------------------------------------- *\
 * screen_pass_shaderf.glsl
 * Author: Gavin Ralston
 * Date Created: 2024-02-26
\* ---------------------------------------------------------------- */
#version 460 core

in vec2 texture_coords;

out vec4 FragColor;

uniform sampler2D screen_texture;

void main()
{
  FragColor = texture(screen_texture, texture_coords);
}
