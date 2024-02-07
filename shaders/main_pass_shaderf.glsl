#version 460 core

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
  readonly uint lod_pool[];
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
  uint stack_trace_octants[64];
  uint stack_trace_indices[64];
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

const float epsilon = 0.001;


vec3 calculateMainRayDirection();
VoxelLocation findVoxelLocation(vec3 world_location, bool clamp_left, bool clamp_bottom, bool clamp_back, out bool is_within_world);
bool rayStep(inout Ray ray);


void main()
{
  Ray ray;
  ray.ray_dir = calculateMainRayDirection();
  bool is_within_world;
  ray.voxel_location = findVoxelLocation(camera_position, ray.ray_dir.x <= 0.0, ray.ray_dir.y <= 0.0, ray.ray_dir.z <= 0.0, is_within_world);
  ray.num_steps = 0;
  ray.distance_traveled = 0.0;

  uint voxel_type;
  bool reached_max_steps = true;
  //bool reached_max_steps = false;
  for (uint i = 0; i < 64; i++)
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
  }
  FragColor = vec4(ray.voxel_location.sublocation, 1.0);
  //FragColor = vec4(vec3(float(ray.num_steps)/(1<<(octree_layers-1))), 1.0);
  //FragColor = vec4(1.0-vec3(ray.distance_traveled/(1<<(octree_layers+1))), 1.0);
  //FragColor = vec4(ray.voxel_location.center/(1<<octree_layers), 1.0);
  //FragColor = vec4(ray.ray_dir, 1.0);
  //FragColor = vec4(float(ray.voxel_location.layer)/float(octree_layers), 0.0, 0.0, 1.0);
  //FragColor = vec4(0.3, 0.7, 0.5, 1.0);
}


vec3 calculateMainRayDirection()
{
  vec3 x = camera_right * screen_position.x;
  vec3 y = camera_up * screen_position.y * float(screen_height) / float(screen_width);
  vec3 z = camera_forward * focal_distance;
  return normalize(x + y + z);
}


VoxelLocation findVoxelLocation(vec3 world_location, bool clamp_left, bool clamp_bottom, bool clamp_back, out bool is_within_world)
{
  VoxelLocation voxel_location;
  voxel_location.stack_trace_octants[0] = 0;
  voxel_location.stack_trace_indices[0] = 0;
  voxel_location.layer = octree_layers;
  voxel_location.type = 0;
  uint current_octree_index = 0;
  vec3 current_octree_center = vec3(0.0, 0.0, 0.0);
  uint current_octree_width = 1 << (octree_layers); // double everything and halve it at the end
  world_location = world_location * 2.0; // doubling
  bool found_uniform = false;
  for (uint i = 0; i < octree_layers-1; i++)
  {
    float octree_epsilon = epsilon;// * float(voxel_location.layer);// / float(voxel_location.layer);
    uint next_octree_width = current_octree_width >> 1;
    vec3 next_octree_center = current_octree_center;
    uint next_octant = 0;

    if (clamp_bottom)
    {
      if (current_octree_center.y - octree_epsilon <= world_location.y)
      {
        next_octree_center.y += next_octree_width >> 1;
        next_octant += 4;
      }
      else
      {
        next_octree_center.y -= next_octree_width >> 1;
      }
    }
    else
    {
      if (current_octree_center.y + octree_epsilon <= world_location.y)
      {
        next_octree_center.y += next_octree_width >> 1;
        next_octant += 4;
      }
      else
      {
        next_octree_center.y -= next_octree_width >> 1;
      }
    }

    if (clamp_back)
    {
      if (current_octree_center.z - octree_epsilon <= world_location.z)
      {
        next_octree_center.z += next_octree_width >> 1;
        next_octant += 2;
      }
      else
      {
        next_octree_center.z -= next_octree_width >> 1;
      }
    }
    else
    {
      if (current_octree_center.z + octree_epsilon <= world_location.z)
      {
        next_octree_center.z += next_octree_width >> 1;
        next_octant += 2;
      }
      else
      {
        next_octree_center.z -= next_octree_width >> 1;
      }
    }

    if (clamp_left)
    {
      if (current_octree_center.x - octree_epsilon <= world_location.x)
      {
        next_octree_center.x += next_octree_width >> 1;
        next_octant += 1;
      }
      else
      {
        next_octree_center.x -= next_octree_width >> 1;
      }
    }
    else
    {
      if (current_octree_center.x + octree_epsilon <= world_location.x)
      {
        next_octree_center.x += next_octree_width >> 1;
        next_octant += 1;
      }
      else
      {
        next_octree_center.x -= next_octree_width >> 1;
      }
    }
    current_octree_index = indirection_pool[(current_octree_index<<3) + next_octant];
    voxel_location.stack_trace_octants[octree_layers-voxel_location.layer] = next_octant;
    voxel_location.stack_trace_indices[octree_layers-voxel_location.layer] = current_octree_index;
    //voxel_location.stack_trace_octants[i] = next_octant;
    //voxel_location.stack_trace_indices[i] = current_octree_index;
    voxel_location.layer -= 1;
    current_octree_center = next_octree_center;
    current_octree_width = next_octree_width;
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
    voxel_location.type = 1;
  }
  voxel_location.center = 0.5 * current_octree_center;
  world_location = 0.5 * world_location;
  voxel_location.sublocation = (world_location - voxel_location.center) * 2.0 / (1 << (voxel_location.layer - 1));
  is_within_world = true;


  /*
  if (abs(voxel_location.sublocation.x) > 1.0 + epsilon || abs(voxel_location.sublocation.y) > 1.0 + epsilon || abs(voxel_location.sublocation.z) > 1.0 + epsilon)
  {
    // This must be outside of the range of the world
    voxel_location.stack_trace_indices[octree_layers-voxel_location.layer-1] = 0;
    voxel_location.type = 0;
    is_within_world = false;
  }
  */
  if (voxel_location.sublocation.x < -1.0) voxel_location.sublocation.x = -1.0;
  else if (voxel_location.sublocation.x > 1.0) voxel_location.sublocation.x = 1.0;
  if (voxel_location.sublocation.y < -1.0) voxel_location.sublocation.y = -1.0;
  else if (voxel_location.sublocation.y > 1.0) voxel_location.sublocation.y = 1.0;
  if (voxel_location.sublocation.z < -1.0) voxel_location.sublocation.z = -1.0;
  else if (voxel_location.sublocation.z > 1.0) voxel_location.sublocation.z = 1.0;


  return voxel_location;
}


bool rayStep(inout Ray ray)
{
  // First, move the the edge of the voxel in the direction of the ray
  // Find the dominating axis (first plane of intersection with the ray)
  float x_distance = 3.0; // Fallback
  if (ray.ray_dir.x > 0.0)
  {
    x_distance = 1.0 - ray.voxel_location.sublocation.x;
  }
  else if (ray.ray_dir.x < 0.0)
  {
    //x_distance = 1.0 + ray.voxel_location.sublocation.x;
    x_distance = -1.0 - ray.voxel_location.sublocation.x;
  }
  float x_factor = x_distance / ray.ray_dir.x;

  float y_distance = 3.0; // Fallback
  if (ray.ray_dir.y > 0.0)
  {
    y_distance = 1.0 - ray.voxel_location.sublocation.y;
  }
  else if (ray.ray_dir.y < 0.0)
  {
    //y_distance = 1.0 + ray.voxel_location.sublocation.y;
    y_distance = -1.0 - ray.voxel_location.sublocation.y;
  }
  float y_factor = y_distance / ray.ray_dir.y;

  float z_distance = 3.0; // Fallback
  if (ray.ray_dir.z > 0.0)
  {
    z_distance = 1.0 - ray.voxel_location.sublocation.z;
  }
  else if (ray.ray_dir.z < 0.0)
  {
    z_distance = -1.0 - ray.voxel_location.sublocation.z;
  }
  float z_factor = z_distance / ray.ray_dir.z;

  // Now find the dominating intersection plane and step forwards to it
  if (x_factor < y_factor && x_factor < z_factor)
  {
    // x is the dominating intersection plane
    ray.voxel_location.sublocation += (x_factor * ray.ray_dir);
    ray.distance_traveled += x_factor * float(1<<ray.voxel_location.layer);
  }
  else if (y_factor < z_factor)
  {
    // y is the dominating intersection plane
    ray.voxel_location.sublocation += (y_factor * ray.ray_dir);
    ray.distance_traveled += y_factor * float(1<<ray.voxel_location.layer);
  }
  else
  {
    // z is the dominating intersection plane
    ray.voxel_location.sublocation += (z_factor * ray.ray_dir);
    ray.distance_traveled += z_factor * float(1<<ray.voxel_location.layer);
  }
  ray.num_steps++;

  // Next, jump to the neighboring voxel
  vec3 world_position = ray.voxel_location.sublocation * float(1 << (ray.voxel_location.layer - 1)) / 2.0 + ray.voxel_location.center;
  bool is_within_world;
  ray.voxel_location = findVoxelLocation(world_position, ray.ray_dir.x > 0.0, ray.ray_dir.y > 0.0, ray.ray_dir.z > 0.0, is_within_world);
  //ray.voxel_location = findVoxelLocation(world_position, ray.ray_dir.x < 0.0, ray.ray_dir.y < 0.0, ray.ray_dir.z < 0.0, is_within_world);

  // Do final checks to return appropriate value
  // return true if the ray has hit a solid block, false otherwise
  if (ray.voxel_location.type == 0)
  {
    // This voxel is uniformly air
    return false;
  }
  return true;
}
