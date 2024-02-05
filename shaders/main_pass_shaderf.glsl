#version 460 core

out vec4 FragColor;

layout (std430, binding = 0) buffer indirection_pool_ssbo
{
  uint indirection_pool[];
};
layout (std430, binding = 1) buffer voxel_type_pool_ssbo
{
  uint voxel_type_pool[];
};
layout (std430, binding = 2) buffer lod_pool_ssbo
{
  uint lod_pool[];
};

uniform int octree_layers;
uniform vec3 camera_position;
uniform float focal_distance;


struct VoxelLocation
{
  uint stack_trace_octants[64];
  uint stack_trace_indices[64];
  uint layer;
  vec3 center;
  vec3 sublocation; // Allows for a point within the voxel - used for casting rays
};

struct Ray
{
  vec3 ray_dir;
  VoxelLocation voxel_location;
  uint num_steps;
};


VoxelLocation findVoxelLocation(vec3 world_location, bool clamp_left, bool clamp_bottom, bool clamp_back);
bool rayStep(inout Ray ray);


void main()
{
  Ray ray;
  ray.ray_dir = normalize(vec3(gl_FragCoord.xy, -focal_distance));
  ray.voxel_location = findVoxelLocation(camera_position, ray.ray_dir.x <= 0.0, ray.ray_dir.y <= 0.0, ray.ray_dir.z <= 0.0);
  ray.num_steps = 0;
  for (uint i = 0; i < 64; i++)
  {
    if (!rayStep(ray))
    {
      break;
    }
  }

  uint voxel_type = voxel_type_pool[ray.voxel_location.stack_trace_indices[ray.voxel_location.layer]];
  //uint voxel_type = indirection_pool[1];
  //FragColor = vec4(vec3(float(ray.num_steps)/64.0), 1.0);
  //FragColor = vec4(ray.voxel_location.center, 1.0);
  if (voxel_type == 0)
  {
    FragColor = vec4(0.0, 0.0, 0.0, 1.0);
  }
  else
  {
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
  }
  //FragColor = vec4(0.3, 0.7, 0.5, 1.0);
}



VoxelLocation findVoxelLocation(vec3 world_location, bool clamp_left, bool clamp_bottom, bool clamp_back)
{
  VoxelLocation voxel_location;
  voxel_location.stack_trace_octants[0] = 0;
  voxel_location.stack_trace_indices[0] = 0;
  voxel_location.layer = octree_layers;
  uint current_octree_index = 0;
  vec3 current_octree_center = vec3(0.0, 0.0, 0.0);
  uint current_octree_width = 2 << (octree_layers - 1);
  for (uint i = 0; i < octree_layers-1; i++)
  {
    uint next_octree_width = current_octree_width >> 1;
    vec3 next_octree_center = current_octree_center;
    uint next_octant = 0;

    if (clamp_bottom)
    {
      if (current_octree_center.y <= world_location.y)
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
      if (current_octree_center.y < world_location.y)
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
      if (current_octree_center.z <= world_location.z)
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
      if (current_octree_center.z < world_location.z)
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
      if (current_octree_center.x <= world_location.x)
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
      if (current_octree_center.x < world_location.x)
      {
        next_octree_center.x += next_octree_width >> 1;
        next_octant += 1;
      }
      else
      {
        current_octree_center.x -= next_octree_width >> 1;
      }
    }
    current_octree_index = indirection_pool[(current_octree_index << 3) + next_octant];
    if (current_octree_index == uint(0))
    {
      break;
    }
    if (voxel_type_pool[current_octree_index] != uint(0))
    {
      break;
    }
    voxel_location.stack_trace_octants[octree_layers-voxel_location.layer] = next_octant;
    voxel_location.stack_trace_indices[octree_layers-voxel_location.layer] = current_octree_index;
    voxel_location.layer -= 1;
    current_octree_center = next_octree_center;
    current_octree_width = next_octree_width;
  }
  voxel_location.center = current_octree_center;
  voxel_location.sublocation = (world_location - current_octree_center) * 2.0 / (2 << (voxel_location.layer - 1));
  return voxel_location;
}


bool rayStep(inout Ray ray)
{
  // Find the neighboring octree
  uint layer = ray.voxel_location.layer;
  uint neighbor_octant = 0;
  uint octant = ray.voxel_location.stack_trace_octants[layer];
  vec3 sublocation = ray.voxel_location.sublocation;
  if (sublocation.y == 1.0)
  {
    neighbor_octant += 18;
  }
  else if (sublocation.y != 1.0)
  {
    neighbor_octant += 9;
  }
  if (sublocation.z == -1.0)
  {
    neighbor_octant += 6;
  }
  else if (sublocation.z != 1.0)
  {
    neighbor_octant += 3;
  }
  if (sublocation.x == 1.0)
  {
    neighbor_octant += 2;
  }
  else if (sublocation.x != -1.0)
  {
    neighbor_octant += 1;
  }

  if (neighbor_octant == 13)
  {
    // This ray is not on the voxel border - so put it there
  }
  VoxelLocation neighbor_location = findVoxelLocation(ray.voxel_location.center + (((sublocation)/2.0) * (2 << (ray.voxel_location.layer-1))), sublocation.x == 1.0, sublocation.y == 1.0, sublocation.z == 1.0);
  ray.voxel_location = neighbor_location;

  // Check if it has hit a solid voxel
  if (voxel_type_pool[ray.voxel_location.stack_trace_indices[octree_layers - ray.voxel_location.layer]] != 0)
  {
    return false;
  }

  // March the ray forward until it hits the boundary of the new voxel
  // NOTE: We are working with sublocations (-1.0 to 1.0) in most of this - not world locations
  float x_distance = 4.0;
  if (ray.ray_dir.x > 0.0)
  {
    x_distance = 1.0 - ray.voxel_location.sublocation.x;
  }
  else if (ray.ray_dir.x < 0.0)
  {
    x_distance = 1.0 + ray.voxel_location.sublocation.x;
  }
  float x_factor = x_distance / ray.ray_dir.x;

  float y_distance = 4.0;
  if (ray.ray_dir.y > 0.0)
  {
    y_distance = 1.0 - ray.voxel_location.sublocation.y;
  }
  else if (ray.ray_dir.y < 0.0)
  {
    y_distance = 1.0 + ray.voxel_location.sublocation.y;
  }
  float y_factor = y_distance / ray.ray_dir.y;

  float z_distance = 4.0;
  if (ray.ray_dir.z > 0.0)
  {
    z_distance = 1.0 - ray.voxel_location.sublocation.z;
  }
  else if (ray.ray_dir.z < 0.0)
  {
    z_distance = 1.0 + ray.voxel_location.sublocation.z;
  }
  float z_factor = z_distance / ray.ray_dir.z;

  // Try X
  Ray tmp_ray = ray;
  tmp_ray.voxel_location.sublocation += (x_factor * tmp_ray.ray_dir);
  if ((abs(tmp_ray.voxel_location.sublocation.y) > 1.0) && !(abs(tmp_ray.voxel_location.sublocation.z) > 1.0))
  {
    // y is the dominating intersection plane
    ray.voxel_location.sublocation += (y_factor * ray.ray_dir);
  }
  else if ((abs(tmp_ray.voxel_location.sublocation.z) > 1.0) && !(abs(tmp_ray.voxel_location.sublocation.y) > 1.0))
  {
    // z is the dominating intersection plane
    ray.voxel_location.sublocation += (z_factor * ray.ray_dir);
  }
  else if ((abs(tmp_ray.voxel_location.sublocation.z) > 1.0) && (abs(tmp_ray.voxel_location.sublocation.y) > 1.0))
  {
    // Try Y
    tmp_ray = ray;
    tmp_ray.voxel_location.sublocation += (y_factor * tmp_ray.ray_dir);
    if (abs(tmp_ray.voxel_location.sublocation.z) > 1.0)
    {
      // z is the dominating intersection plane
      ray.voxel_location.sublocation += (z_factor * ray.ray_dir);
    }
    else
    {
      // y is the dominating intersection plane
      ray.voxel_location.sublocation += (y_factor * ray.ray_dir);
    }
  }
  else if (!(abs(tmp_ray.voxel_location.sublocation.z) > 1.0) && !(abs(tmp_ray.voxel_location.sublocation.y) > 1.0))
  {
    // x is the dominating intersection plane
    ray.voxel_location.sublocation += (x_factor * ray.ray_dir);
  }

  ray.num_steps += 1;
  return true;
}
