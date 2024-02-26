/* ---------------------------------------------------------------- *\
 * main_pass_shaderf.glsl
 * Author: Gavin Ralston
 * Date Created: 2024-02-03
\* ---------------------------------------------------------------- */
#version 430 core
//#version 460 core
//#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

#define MAX_OCTREE_LAYERS 64

in vec2 screen_position;
out vec4 FragColor;

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

uniform int octree_layers;
uniform float focal_distance;
uniform int screen_width;
uniform int screen_height;
uniform vec3 camera_position;
uniform vec3 camera_right;
uniform vec3 camera_up;
uniform vec3 camera_forward;


struct VoxelLocation
{
  uint stack_trace_octants[MAX_OCTREE_LAYERS];
  uint stack_trace_indices[MAX_OCTREE_LAYERS];
  uint layer;
  vec3 center;
  vec3 sublocation; // Allows for a point within the voxel - mainly used for casting rays
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


vec3 calculateMainRayDirection();
uint stepToEdge(inout Ray ray);
VoxelLocation findVoxelLocation(vec3 world_location);
uint createTargetVoxelTraceSingleAxisStep(inout float position, float radius);
//bool jumpToNeighbor(inout VoxelLocation voxel_location, uint neighbor);
bool jumpToNeighbor(inout Ray ray, uint neighbor);
bool rayStep(inout Ray ray);
uint computeLOD(float dist);


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
    if (rayStep(ray))
    {
      break;
    }
  }
  voxel_type = ray.voxel_location.type;

  //uint voxel_type = indirection_pool[3];
  if (voxel_type == 0)
  {
    FragColor = vec4(0.0, 0.0, 0.0, 1.0);
  }
  else
  {
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    //FragColor = vec4(ray.voxel_location.sublocation, 1.0);
    //FragColor = vec4(1.0-vec3(ray.distance_traveled/(1<<(octree_layers+1))), 1.0);
  }
  FragColor = vec4(ray.voxel_location.sublocation, 1.0);
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
}


vec3 calculateMainRayDirection()
{
  vec3 x = camera_right * screen_position.x;
  vec3 y = camera_up * screen_position.y * float(screen_height) / float(screen_width);
  vec3 z = camera_forward * focal_distance;
  return normalize(x + y + z);
}


uint stepToEdge(inout Ray ray)
{
  // Step to the edge of the current voxel, in the direction of the ray
  // Return the new intersecting plane (0=-x, 1=+x, 2=-y, 3=+y, 4=-z, 5=+z)
  // First, find the dominating axis (first plane of intersection with the ray)
  // Note that the sublocation coordinates are normalized between -1.0 and 1.0, so we can use the sign function here
  float x_factor = (sign(ray.ray_dir.x) - ray.voxel_location.sublocation.x) / ray.ray_dir.x;
  float y_factor = (sign(ray.ray_dir.y) - ray.voxel_location.sublocation.y) / ray.ray_dir.y;
  float z_factor = (sign(ray.ray_dir.z) - ray.voxel_location.sublocation.z) / ray.ray_dir.z;

  // Now find the dominating intersection plane and step forwards to it
  float neighbor_direction;
  if (x_factor < y_factor && x_factor < z_factor)
  {
    // x is the dominating intersection plane
    ray.voxel_location.sublocation += (x_factor * ray.ray_dir);
    ray.distance_traveled += x_factor * float(1<<ray.voxel_location.layer);
    neighbor_direction = (sign(ray.ray_dir.x) + 1.0) * 0.5;
  }
  else if (y_factor < z_factor)
  {
    // y is the dominating intersection plane
    ray.voxel_location.sublocation += (y_factor * ray.ray_dir);
    ray.distance_traveled += y_factor * float(1<<ray.voxel_location.layer);
    neighbor_direction = (sign(ray.ray_dir.y) + 1.0) * 0.5 + 2.0;
  }
  else
  {
    // z is the dominating intersection plane
    ray.voxel_location.sublocation += (z_factor * ray.ray_dir);
    ray.distance_traveled += z_factor * float(1<<ray.voxel_location.layer);
    neighbor_direction = (sign(ray.ray_dir.z) + 1.0) * 0.5 + 4.0;
  }
  ray.num_steps++;

  return uint(neighbor_direction);
}


VoxelLocation findVoxelLocation(vec3 world_location)
{
  VoxelLocation voxel_location;
  voxel_location.center = vec3(0.0, 0.0, 0.0);
  voxel_location.layer = octree_layers;
  uint current_octree_index = 0;
  bool found_uniform = false;
  float radius = float(1 << (octree_layers-1)) / 2;
  vec3 world_location_copy = world_location;
  for (uint i = 0; i < octree_layers-1; i++)
  {
    uint x_target_trace = createTargetVoxelTraceSingleAxisStep(world_location_copy.x, radius);
    uint y_target_trace = createTargetVoxelTraceSingleAxisStep(world_location_copy.y, radius);
    uint z_target_trace = createTargetVoxelTraceSingleAxisStep(world_location_copy.z, radius);
    radius *= 0.5;
    voxel_location.stack_trace_octants[i] = 0;
    voxel_location.stack_trace_octants[i] += x_target_trace;
    voxel_location.stack_trace_octants[i] += 4 * y_target_trace;
    voxel_location.stack_trace_octants[i] += 2*(1-z_target_trace);
    voxel_location.center.x += x_target_trace * radius * 2 - radius;
    voxel_location.center.y += y_target_trace * radius * 2 - radius;
    voxel_location.center.z += z_target_trace * radius * 2 - radius;

    current_octree_index = indirection_pool[(current_octree_index<<3) + voxel_location.stack_trace_octants[i]];
    voxel_location.stack_trace_indices[i] = current_octree_index;
    voxel_location.layer -= 1;
    if (current_octree_index == uint(0))
    {
      // This entire octree is air
      voxel_location.type = 0;
      found_uniform = true;
      break;
    }
    if (voxel_type_pool[current_octree_index] != uint(0))
    {
      // This entire octree is uniform (not air)
      voxel_location.type = 1;
      found_uniform = true;
      break;
    }
  }

  if (!found_uniform)
  {
    // The maximum number of traversals have been made but there are still more children to this node.
    // If intentional, this is probably supposed to be a fractal.
    voxel_location.type = 1;
  }
  bool is_within_world = true;

  voxel_location.sublocation = (world_location - voxel_location.center) * 2.0 / (1 << (voxel_location.layer - 1));

  return voxel_location;
}


uint createTargetVoxelTraceSingleAxisStep(inout float position, float radius)
{
  /*
  radius *= 0.5;
  if (position > 0.0)
  {
    position -= radius;
    return 1;
  }
  else
  {
    position += radius;
    return 0;
  }
  */
  float move = 0.5*radius*sign(position);
  position -= move;
  return uint((sign(move) + 1.0)*0.5);
}


bool jumpToNeighbor(inout Ray ray, uint neighbor)
{
  // NOTE: This does not correctly compute the center of the new voxel location.
  //
  // Jump to the specified neighbor without changing the world position
  // It's done this way in an effor to minimize floating point errors

  // neighbor_octant values:
  // 0 = left (-x)
  // 1 = right (+x)
  // 2 = bottom (-y)
  // 3 = top (+y)
  // 4 = back (-z)
  // 5 = front(+z)

  uint old_layer = ray.voxel_location.layer;

  // First, find the path (stack_trace_octants) to the neighboring voxel (this may be incomplete or overcomplete)
  // format: sign * mod(target_path[i], mod_value) < comp_value
  // 0 -> 1 - mod(target_path[i], 2) < 1
  // 1 -> 0 + mod(target_path[i], 2) < 1
  // 2 -> 7 - mod(target_path[i], 8) < 4
  // 3 -> 0 + mod(target_path[i], 8) < 4
  // 4 -> 0 + mod(target_path[i], 4) < 2
  // 5 -> 3 - mod(target_path[i], 4) < 2
  //
  // 0 -> -mod(target_path[i], 2) < 0
  // 1 -> mod(target_path[i], 2) < 1
  // 2 -> -mod(target_path[i], 8) < -3
  // 3 -> mod(target_path[i], 8) < 4
  // 4 -> mod(target_path[i], 4) < 2
  // 5 -> -mod(target_path[i], 4) < -1
  //
  // sign
  // 0 -> -1
  // 1 -> 1
  // 2 -> -1
  // 3 -> 1
  // 4 -> 1
  // 5 -> -1
  // mod_value
  // 0 -> 2
  // 1 -> 2
  // 2 -> 8
  // 3 -> 8
  // 4 -> 4
  // 5 -> 4
  // comp_value
  // 0 -> 0
  // 1 -> 1
  // 2 -> -4
  // 3 -> 4
  // 4 -> 2
  // 5 -> -2
  int sign0, mod_value, comp_value, add_value;

  uvec3 constant, multiplier; // used for the next step
  if (neighbor == 0)
  {
    sign0 = -1;
    mod_value = 2;
    comp_value = 0;
    add_value = -1;
    constant = uvec3(1, 0, 0);
    multiplier = uvec3(0, 4, 2);
  }
  else if (neighbor == 1)
  {
    sign0 = 1;
    mod_value = 2;
    comp_value = 1;
    add_value = 1;
    constant = uvec3(0, 0, 0);
    multiplier = uvec3(0, 4, 2);
  }
  else if (neighbor == 2)
  {
    sign0 = -1;
    mod_value = 8;
    comp_value = -3;
    add_value = -4;
    constant = uvec3(0, 4, 0);
    multiplier = uvec3(1, 0, 2);
  }
  else if (neighbor == 3)
  {
    sign0 = 1;
    mod_value = 8;
    comp_value = 4;
    add_value = 4;
    constant = uvec3(0, 0, 0);
    multiplier = uvec3(1, 0, 2);
  }
  else if (neighbor == 4)
  {
    sign0 = 1;
    mod_value = 4;
    comp_value = 2;
    add_value = 2;
    constant = uvec3(0, 0, 0);
    multiplier = uvec3(1, 4, 0);
  }
  else if (neighbor == 5)
  {
    sign0 = -1;
    mod_value = 4;
    comp_value = -1;
    add_value = -2;
    constant = uvec3(0, 0, 2);
    multiplier = uvec3(1, 4, 0);
  }
  uint i;
  uint last_unchanged_layer = ray.voxel_location.layer+1;
  bool found = false;
  // Find the last odd number, subtract 1 from it, then add 1 to all following numbers
  for (i = ray.voxel_location.layer; i < octree_layers; i++)
  {
    if (sign0 * mod(ray.voxel_location.stack_trace_octants[octree_layers-i-1], mod_value) < comp_value)
    {
      ray.voxel_location.stack_trace_octants[octree_layers-i-1] += add_value;
      found = true;
      break;
    }
    else
      last_unchanged_layer++;
      ray.voxel_location.stack_trace_octants[octree_layers-i-1] -= add_value;
  }
  if (!found)
  {
    return false;
  }
  uint last_unchanged_index = octree_layers-i-1;
  i = ray.voxel_location.layer-1;
  vec3 voxel_sublocation_copy = ray.voxel_location.sublocation;
  float radius = 1.0;

  for (uint j = 0; i >= 1; i--, j++)
  {
    uint next_x_trace = createTargetVoxelTraceSingleAxisStep(voxel_sublocation_copy.x, radius);
    uint next_y_trace = createTargetVoxelTraceSingleAxisStep(voxel_sublocation_copy.y, radius);
    uint next_z_trace = 1-createTargetVoxelTraceSingleAxisStep(voxel_sublocation_copy.z, radius);
    radius *= 0.5;
    ray.voxel_location.stack_trace_octants[octree_layers-i-1] = next_x_trace*multiplier.x + constant.x;
    ray.voxel_location.stack_trace_octants[octree_layers-i-1] += next_y_trace*multiplier.y + constant.y;
    ray.voxel_location.stack_trace_octants[octree_layers-i-1] += next_z_trace*multiplier.z + constant.z;

    /*
    ray.voxel_location.center.x += next_x_trace*multiplier.x*radius*2 - radius;
    ray.voxel_location.center.y += next_x_trace*multiplier.y*radius*2 - radius;
    ray.voxel_location.center.z += next_x_trace*multiplier.z*radius*2 - radius;
    */
  }

  //voxel_location.layer = octree_layers;
  ray.voxel_location.layer = last_unchanged_layer;
  ray.voxel_location.center = vec3(0.0, 0.0, 0.0);
  radius = float(1 << (octree_layers-1)) * 0.5;
  uint current_octree_index = ray.voxel_location.stack_trace_indices[last_unchanged_index];
  bool found_uniform = false;
  uint lod_layer = computeLOD(ray.distance_traveled);
  for (uint i = last_unchanged_index; i < octree_layers-1; i++)
  {
    /*
    radius *= 0.5;
    vec3 multiplier = vec3(0.0);
    if (mod(target_path[i], 2) == 1.0)
      multiplier.x = 1.0;
    if (target_path[i] >= 4.0)
      multiplier.y = 1.0;
    if (mod(target_path[i], 4) >= 2.0)
      multiplier.z = 0.0;
    voxel_location.center += multiplier * radius * 2 - radius;
    */

    current_octree_index = indirection_pool[(current_octree_index<<3) + ray.voxel_location.stack_trace_octants[i]];
    ray.voxel_location.stack_trace_indices[i] = current_octree_index;
    ray.voxel_location.layer--;
    if (current_octree_index == uint(0))
    {
      // This entire octree is air
      ray.voxel_location.type = 0;
      found_uniform = true;
      break;
    }
    if (voxel_type_pool[current_octree_index] != uint(0))
    {
      // This entire octree is uniform (not air)
      ray.voxel_location.type = 1;
      found_uniform = true;
      break;
    }
    if (ray.voxel_location.layer <= lod_layer)
    {
      // This octree is far enough away that we can use the current octree for LOD
      ray.voxel_location.type = 1;
      break;
    }
  }

  if (!found_uniform)
  {
    ray.voxel_location.type = 1;
  }

  // Calculate new sublocation
  // Start at the current sublocation
  // Flip the sign of the axis we've jumped across and snap to -1.0 or +1.0
  if (neighbor <= 1)
  {
    ray.voxel_location.sublocation.x = -sign(ray.voxel_location.sublocation.x);
  }
  else if (neighbor <= 3)
  {
    ray.voxel_location.sublocation.y = -sign(ray.voxel_location.sublocation.y);
  }
  else
  {
    ray.voxel_location.sublocation.z = -sign(ray.voxel_location.sublocation.z);
  }

  if (ray.voxel_location.layer < old_layer)
  {
    // Recurse further down the octree
    for (uint i = old_layer; i > ray.voxel_location.layer; i--)
    {
      vec3 modifier;
      modifier.x = float((ray.voxel_location.stack_trace_octants[octree_layers-i] % 2));
      modifier.y = float((ray.voxel_location.stack_trace_octants[octree_layers-i] % 8) / 4);
      modifier.z = float(1 - ((ray.voxel_location.stack_trace_octants[octree_layers-i] % 4) / 2));

      ray.voxel_location.sublocation = 2.0*ray.voxel_location.sublocation + 1.0 - 2.0*modifier;
    }
  }
  else if (ray.voxel_location.layer > old_layer)
  {
    // Recurse up the octree
    for (uint i = old_layer; i < ray.voxel_location.layer; i++)
    {
      vec3 modifier;
      modifier.x = float((ray.voxel_location.stack_trace_octants[octree_layers-i-1] % 2));
      modifier.y = float((ray.voxel_location.stack_trace_octants[octree_layers-i-1] % 8) / 4);
      modifier.z = float(1 - ((ray.voxel_location.stack_trace_octants[octree_layers-i-1] % 4) / 2));

      ray.voxel_location.sublocation = 0.5 * (ray.voxel_location.sublocation - 1.0) + modifier;
    }
  }
  return true;
}


bool rayStep(inout Ray ray)
{
  // First, move the the edge of the voxel in the direction of the ray
  uint neighbor = stepToEdge(ray);

  // Next, jump to the neighboring voxel
  bool in_bounds = jumpToNeighbor(ray, neighbor);
  if (!in_bounds)
  {
    ray.voxel_location.type = 0;
    return true;
  }

  // Do final checks to return appropriate value
  // return true if the ray has hit a solid block, false otherwise
  if (ray.voxel_location.type == 0)
  {
    // This voxel is uniformly air
    return false;
  }
  return true;
}


uint computeLOD(float dist)
{
  // Compute the lowest reasonable octree layer to use based on the given distance
  uint num_pixels = max(screen_width, screen_height);
  uint num_visible_pixels = uint(dist / focal_distance);
  return uint(log2(num_visible_pixels / num_pixels));
  //return uint(dist / 1000) + 1;
}
