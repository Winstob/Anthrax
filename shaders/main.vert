/* ---------------------------------------------------------------- *\
 * main.vert
 * Author: Gavin Ralston
 * Date Created: 2024-03-31
\* ---------------------------------------------------------------- */
#version 460

layout (location = 0) out vec3 color;

vec2 positions[3] = vec2[](
  vec2(-1.0, -1.0),
  vec2(3.0, -1.0),
  vec2(-1.0, 3.0)
);

vec3 colors[3] = vec3[](
  vec3(1.0, 0.0, 0.0),
  vec3(0.0, 1.0, 0.0),
  vec3(0.0, 0.0, 1.0)
);


void main()
{
  gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
  color = colors[gl_VertexIndex];
}

