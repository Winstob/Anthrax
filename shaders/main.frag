/* ---------------------------------------------------------------- *\
 * main.frag
 * Author: Gavin Ralston
 * Date Created: 2024-03-31
\* ---------------------------------------------------------------- */
#version 460

layout (location = 0) in vec3 color;

layout (location = 0) out vec4 FragColor;

layout (std140, binding = 0) readonly buffer buffer0
{
	float raymarched_image[];
};


void main()
{
  //FragColor = vec4(color, 1.0);
	FragColor = vec4(vec3(raymarched_image[0]), 1.0);
}
