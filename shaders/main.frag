/* ---------------------------------------------------------------- *\
 * main.frag
 * Author: Gavin Ralston
 * Date Created: 2024-03-31
\* ---------------------------------------------------------------- */
#version 460

layout (location = 0) in vec3 color;

layout (location = 0) out vec4 FragColor;

layout (rgba32f, binding = 0) uniform readonly image2D image0;


void main()
{
  //FragColor = vec4(color, 1.0);
	//FragColor = vec4(vec3(raymarched_image[0]), 1.0);
	//FragColor = vec4(gl_FragCoord.x/800.0, gl_FragCoord.y/600.0, 0.0, 1.0);
	FragColor = imageLoad(image0, ivec2(gl_FragCoord.xy));
	return;
}
