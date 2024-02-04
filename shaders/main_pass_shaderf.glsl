#version 460 core

out vec4 FragColor;

layout (std430, binding = 0) buffer indirection_pool
{
  uint children[8];
};
layout (std430, binding = 1) buffer voxel_type_pool
{
  uint voxel_type;
};
layout (std430, binding = 2) buffer lod_pool
{
  uint color_value;
};

void main()
{
  //FragColor = vec4(1.0);
  FragColor = vec4(0.3, 0.7, 0.5, 1.0);
}
