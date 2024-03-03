/* ---------------------------------------------------------------- *\
 * main_pass_shaderf.glsl
 * Author: Gavin Ralston
 * Date Created: 2024-02-03
\* ---------------------------------------------------------------- */
#version 460 core
//#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

in vec2 screen_position;
//out vec4 FragColor;
layout (location = 0) out vec4 FragColor;


layout (std430, binding = 0) buffer indirection_pool_ssbo
{
  readonly uint indirection_pool[];
};
layout (std430, binding = 1) buffer voxel_type_pool_ssbo
{
  readonly uint voxel_type_pool[];
};
layout (std430, binding = 2) buffer lod_pool_ssbo
{
  readonly uint lod_pool[]; // Stored as a compressed vector of 4 8-bit unsigned integers
};

struct WorldLocation
{
  ivec3 position;
  vec3 sublocation;
};

uniform int octree_layers;
uniform float focal_distance;
uniform int screen_width;
uniform int screen_height;
uniform WorldLocation camera_position;
uniform vec3 camera_right;
uniform vec3 camera_up;
uniform vec3 camera_forward;

struct VoxelLocation
{
  dvec3 position; // The position of the voxel where the origin is at the corner of the world (not the center)
  uint layer;
                    // Normalized coordinates between -1.0 and 1.0 in each axis, regardless of layer
  uint type;
};

struct Ray
{
  vec3 ray_dir;
  VoxelLocation voxel_location;
  uint num_steps;
  float distance_traveled;
};

const float epsilon = 0.0;
const float render_distance = 100.0 * 1000 * 10;
//const float render_distance = 1024.0;
const uint lod_multiplier = 1;


vec3 calculateMainRayDirection();
VoxelLocation findVoxelLocation(WorldLocation world_location);
bool rayStep(inout Ray ray);


void main()
{
  Ray ray;
  ray.ray_dir = calculateMainRayDirection();
  bool is_within_world;
  ray.voxel_location = findVoxelLocation(camera_position);
  ray.num_steps = 0;
  ray.distance_traveled = 0.0;

  uint voxel_type;
  bool reached_max_steps = true;
  //bool reached_max_steps = false;
  for (uint i = 0; i < pow(2, octree_layers); i++)
  {
    //if (rayStep(ray) || ray.distance_traveled >= render_distance)
    if (rayStep(ray))
    {
      break;
    }
  }
  /*
  FragColor = vec4(ray.voxel_location.position/pow(2, octree_layers-1), 1.0);
  return;
  */
  voxel_type = ray.voxel_location.type;

  //uint voxel_type = indirection_pool[3];
  if (voxel_type == 0)
  {
    FragColor = vec4(0.0, 0.0, 0.0, 1.0);
  }
  else
  {
    if (voxel_type == 2)
      FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    else
      FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    //FragColor = vec4(vec3(float(ray.num_steps)/2.0), 1.0);
    //FragColor = vec4(ray.voxel_location.sublocation, 1.0);
    //FragColor = vec4(1.0-vec3(ray.distance_traveled/(1<<(octree_layers+1))), 1.0);
  }
  //FragColor = vec4(ray.voxel_location.position/float(ray.num_steps), 1.0);
  //FragColor = vec4(ray.voxel_location.sublocation, 1.0);
  //FragColor = vec4(vec3(float(ray.num_steps)/(1<<(octree_layers-1))), 1.0);
  //FragColor = vec4(ray.voxel_location.center/(1<<octree_layers), 1.0);
  //FragColor = vec4(ray.ray_dir, 1.0);
  //FragColor = vec4(float(ray.voxel_location.layer)/float(octree_layers), 0.0, 0.0, 1.0);
  //FragColor = vec4(0.3, 0.7, 0.5, 1.0);

  // cel shading
  /*
  float eps = 1/ray.voxel_location.layer * 0.05;
  if ((abs(ray.voxel_location.sublocation.x) > 1.0-eps && (abs(ray.voxel_location.sublocation.y) > 1.0-eps || abs(ray.voxel_location.sublocation.z) > 1.0-eps)) || (abs(ray.voxel_location.sublocation.y) > 1.0-eps && abs(ray.voxel_location.sublocation.z) > 1.0-eps))
  {
    FragColor = vec4(vec3(0.0), 1.0);
  }
  */
  //FragColor = FragColor * max((1-ray.distance_traveled/render_distance), 0.0); // Fog
}


vec3 calculateMainRayDirection()
{
  vec3 x = camera_right * screen_position.x;
  vec3 y = camera_up * screen_position.y * float(screen_height) / float(screen_width);
  vec3 z = camera_forward * focal_distance;
  return normalize(x + y + z);
}


VoxelLocation findVoxelLocation(WorldLocation world_location)
{
  VoxelLocation voxel_location;

  uvec3 int_position;
  uint mask = uint(0x7FFFFFFF) >> (32-octree_layers+1);

  uint sign0 = (uint(world_location.position.x) & 0x80000000u) >> 31;
  sign0 = (~sign0) & 0x1u;
  sign0 = sign0 << (octree_layers-2);
  int_position.x = (uint(world_location.position.x) & mask) | sign0;

  sign0 = (uint(world_location.position.y) & 0x80000000u) >> 31;
  sign0 = (~sign0) & 0x1u;
  sign0 = sign0 << (octree_layers-2);
  int_position.y = (uint(world_location.position.y) & mask) | sign0;

  sign0 = (uint(world_location.position.z) & 0x80000000u) >> 31;
  sign0 = (~sign0) & 0x1u;
  sign0 = sign0 << (octree_layers-2);
  int_position.z = (uint(world_location.position.z) & mask) | sign0;

  voxel_location.position = dvec3(int_position) + world_location.sublocation;

  voxel_location.layer = octree_layers;
  uint current_octree_index = 0;
  bool found_uniform = false;
  for (uint i = 0; i < octree_layers-1; i++)
  {
    
    mask = uint(1) << (octree_layers-i-2);
    uint current_octant = ((int_position.x & mask) >> (octree_layers-i-2)) | (((int_position.y & mask) >> (octree_layers-i-2)) << 2) | (((int_position.z & mask) >> (octree_layers-i-2)) << 1);

    current_octree_index = indirection_pool[(current_octree_index<<3) | current_octant];
    voxel_location.layer--;

    voxel_location.type = voxel_type_pool[current_octree_index];
    if (voxel_location.type != 0 || current_octree_index == 0)
    {
      found_uniform = true;
      break;
    }
  }
  if (!found_uniform)
  {
    voxel_location.type = 1;
  }
  return voxel_location;
}


bool rayStep(inout Ray ray)
{
  float magnitude = 0.5;
  ray.voxel_location.position = dvec3(ray.ray_dir)*magnitude + ray.voxel_location.position;
  double max_position = double(uint(0xFFFFFFFF) >> (32-octree_layers+1));
  if (ray.voxel_location.position.x > max_position || ray.voxel_location.position.y > max_position || ray.voxel_location.position.z > max_position || ray.voxel_location.position.x < 0.0 || ray.voxel_location.position.y < 0.0 || ray.voxel_location.position.z < 0.0)
  {
    ray.voxel_location.type = 2;
    return true;
  }


  uvec3 int_position = uvec3(ray.voxel_location.position);

  ray.voxel_location.layer = octree_layers;
  uint current_octree_index = 0;
  bool found_uniform = false;
  for (uint i = 0; i < octree_layers-1; i++)
  {
    ray.voxel_location.layer--;

    uint mask = 1u << (octree_layers-i-2);
    uint current_octant = ((int_position.x & mask) >> (octree_layers-i-2)) | (((int_position.y & mask) >> (octree_layers-i-2)) << 2) | (((int_position.z & mask) >> (octree_layers-i-2)) << 1);
    //current_octant = min(current_octant, 7);
    //current_octant = 4;

    current_octree_index = indirection_pool[(current_octree_index<<3) | current_octant];

    if (current_octree_index == 0)
    {
      found_uniform = true;
      break;
    }

    ray.voxel_location.type = voxel_type_pool[current_octree_index];
    if (ray.voxel_location.type != 0)
    {
      found_uniform = true;
      break;
    }
  }
  if (!found_uniform)
  {
    ray.voxel_location.type = 1;
    return true;
  }

  ray.num_steps++;
  return ray.voxel_location.type != 0u;
}
