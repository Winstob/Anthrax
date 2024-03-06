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
  ivec3 int_component;
  vec3 dec_component;
};


struct uinfl
{
  uint int_component;
  float dec_component;
};
uinfl uinflAdd(uinfl first, uinfl second);
uinfl uinflSub(uinfl first, uinfl second);
//double double(uinfl x) { return double(x.int_component) + double(x.dec_component); }

struct ufvec3
{
  uvec3 int_component;
  vec3 dec_component;
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
  ufvec3 position; // The position of the voxel where the origin is at the corner of the world (not the center)
  uint layer;
  uint type;
};

struct Ray
{
  vec3 ray_dir;
  VoxelLocation voxel_location;
  uint num_steps;
  double distance_traveled;
};

const float epsilon = 0.000001;
const float render_distance = 100.0 * 1000 * 10;
//const float render_distance = 1024.0;
const uint lod_multiplier = 1;


vec3 calculateMainRayDirection();
VoxelLocation findVoxelLocation(WorldLocation world_location);
double findSafeDistance(in Ray ray);
bool oldRayMarch(inout Ray ray);
bool rayMarch(inout Ray ray);


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
  //for (uint i = 0; i < 500; i++)
  {
    //if (rayMarch(ray) || ray.distance_traveled >= render_distance)
    //if (oldRayMarch(ray))
    if (rayMarch(ray))
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
    /*
    if (voxel_type == 2)
      FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    else
      FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    */
    //FragColor = vec4(ray.voxel_location.position-uvec3(ray.voxel_location.position), 1.0);
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    //FragColor = vec4(vec3(float(ray.num_steps)/2.0), 1.0);
    //FragColor = vec4(ray.voxel_location.sublocation, 1.0);
    //FragColor = vec4(1.0-vec3(ray.distance_traveled/(1<<(octree_layers+1))), 1.0);
  }
  FragColor = vec4(ray.voxel_location.position.dec_component, 1.0);
  //FragColor = vec4(vec3(ray.distance_traveled / pow(2, octree_layers)), 1.0);
  //FragColor = vec4(vec3(float(ray.num_steps) / 8.0), 1.0);
  //FragColor = vec4(vec3(ray.voxel_location.position.int_component)/500.0, 1.0);
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

  voxel_location.position.int_component = uvec3(world_location.int_component) + uvec3(0x1 << (octree_layers-2));

  voxel_location.position.dec_component = world_location.dec_component;
  if (voxel_location.position.dec_component.x < 0.0)
  {
    voxel_location.position.dec_component.x += 1.0;
    voxel_location.position.int_component.x -= 1;
  }
  if (voxel_location.position.dec_component.y < 0.0)
  {
    voxel_location.position.dec_component.y += 1.0;
    voxel_location.position.int_component.y -= 1;
  }
  if (voxel_location.position.dec_component.z < 0.0)
  {
    voxel_location.position.dec_component.z += 1.0;
    voxel_location.position.int_component.z -= 1;
  }

  //voxel_location.position = dvec3(voxel_location.position.int_component) + voxel_location.position.dec_component;

  uvec3 int_position = voxel_location.position.int_component;

  voxel_location.layer = octree_layers;
  uint current_octree_index = 0;
  bool found_uniform = false;
  for (uint i = 0; i < octree_layers-1; i++)
  {
    
    uint mask = uint(1) << (octree_layers-i-2);
    uint current_octant = ((int_position.x & mask) >> (octree_layers-i-2)) | (((int_position.y & mask) >> (octree_layers-i-2)) << 2) | (((~int_position.z & mask) >> (octree_layers-i-2)) << 1);

    current_octree_index = indirection_pool[(current_octree_index<<3) | current_octant];
    voxel_location.layer--;

    voxel_location.type = voxel_type_pool[current_octree_index];
    if (voxel_location.type != 0)
    {
      found_uniform = true;
      break;
    }
    if (current_octree_index == 0)
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


uint stepToEdge(inout Ray ray)
{
  // Takes in ray and steps it in the specified direction until it hits the edge of the voxel it is
  // currently in (based on the layer). Outputs the neighboring voxel:
  // -x = 0
  // +x = 1
  // -y = 2
  // +y = 3
  // -z = 4
  // +z = 5
  uint sublocation_mask = uint(0xFFFFFFFF) >> (32-ray.voxel_location.layer+1);
  if (ray.voxel_location.layer == 1) sublocation_mask = 0u;
  //float radius = float(1u << (ray.voxel_location.layer-2));
  uinfl max_sublocation = {sublocation_mask, 1.0f};
  uinfl min_sublocation = {0, 0.0f};
  uinfl x_sublocation, y_sublocation, z_sublocation;
  x_sublocation.int_component = ray.voxel_location.position.int_component.x & sublocation_mask;
  x_sublocation.dec_component = ray.voxel_location.position.dec_component.x;
  y_sublocation.int_component = ray.voxel_location.position.int_component.y & sublocation_mask;
  y_sublocation.dec_component = ray.voxel_location.position.dec_component.y;
  z_sublocation.int_component = ray.voxel_location.position.int_component.z & sublocation_mask;
  z_sublocation.dec_component = ray.voxel_location.position.dec_component.z;
  //ufvec3 sublocation;
  //sublocation.int_component = ray.voxel_location.position.int_component & sublocation_mask;
  //sublocation.dec_component = ray.voxel_location.position.dec_component;

  uinfl x_distance = max_sublocation, y_distance = max_sublocation, z_distance = max_sublocation;
  if (ray.ray_dir.x < 0.0)
    x_distance = x_sublocation;
  else if (ray.ray_dir.x > 0.0)
    x_distance = uinflSub(max_sublocation, x_sublocation);
  if (ray.ray_dir.y < 0.0)
    y_distance = y_sublocation;
  else if (ray.ray_dir.y > 0.0)
    y_distance = uinflSub(max_sublocation, y_sublocation);
  if (ray.ray_dir.z < 0.0)
    z_distance = z_sublocation;
  else if (ray.ray_dir.z > 0.0)
    z_distance = uinflSub(max_sublocation, z_sublocation);


  double tmp = double(x_distance.int_component) + x_distance.dec_component;
  double x_factor = tmp / abs(ray.ray_dir.x);
  tmp = double(y_distance.int_component) + y_distance.dec_component;
  double y_factor = tmp / abs(ray.ray_dir.y);
  tmp = double(z_distance.int_component) + z_distance.dec_component;
  double z_factor = tmp / abs(ray.ray_dir.z);

  uint neighbor;
  if (x_factor < y_factor && x_factor < z_factor)
  {
    // x is the dominating axis
    if (ray.ray_dir.x < 0.0)
    {
      neighbor = 0;
      x_sublocation = min_sublocation;
    }
    else
    {
      neighbor = 1;
      x_sublocation = max_sublocation;
    }
    tmp = double(y_sublocation.int_component) + y_sublocation.dec_component;
    tmp += ray.ray_dir.y * x_factor;
    y_sublocation.int_component = uint(tmp);
    y_sublocation.dec_component = float(tmp - y_sublocation.int_component);

    tmp = double(z_sublocation.int_component) + z_sublocation.dec_component;
    tmp += ray.ray_dir.z * x_factor;
    z_sublocation.int_component = uint(tmp);
    z_sublocation.dec_component = float(tmp - z_sublocation.int_component);

    ray.distance_traveled += x_factor;
  }
  else if (y_factor < z_factor)
  {
    // y is the dominating axis
    if (ray.ray_dir.y < 0.0)
    {
      neighbor = 2;
      y_sublocation = min_sublocation;
    }
    else
    {
      neighbor = 3;
      y_sublocation = max_sublocation;
    }
    tmp = double(x_sublocation.int_component) + x_sublocation.dec_component;
    tmp += ray.ray_dir.x * y_factor;
    x_sublocation.int_component = uint(tmp);
    x_sublocation.dec_component = float(tmp - x_sublocation.int_component);

    tmp = double(z_sublocation.int_component) + z_sublocation.dec_component;
    tmp += ray.ray_dir.z * y_factor;
    z_sublocation.int_component = uint(tmp);
    z_sublocation.dec_component = float(tmp - z_sublocation.int_component);

    ray.distance_traveled += y_factor;
  }
  else
  {
    // z is the dominating axis
    if (ray.ray_dir.z < 0.0)
    {
      neighbor = 4;
      z_sublocation = min_sublocation;
    }
    else
    {
      neighbor = 5;
      z_sublocation = max_sublocation;
    }
    tmp = double(x_sublocation.int_component) + x_sublocation.dec_component;
    tmp += ray.ray_dir.x * z_factor;
    x_sublocation.int_component = uint(tmp);
    x_sublocation.dec_component = float(tmp - x_sublocation.int_component);

    tmp = double(y_sublocation.int_component) + y_sublocation.dec_component;
    tmp += ray.ray_dir.y * z_factor;
    y_sublocation.int_component = uint(tmp);
    y_sublocation.dec_component = float(tmp - y_sublocation.int_component);

    ray.distance_traveled += z_factor;
  }

  ray.voxel_location.position.int_component.x = (ray.voxel_location.position.int_component.x & (~sublocation_mask)) | (x_sublocation.int_component & sublocation_mask);
  ray.voxel_location.position.dec_component.x = x_sublocation.dec_component;
  ray.voxel_location.position.int_component.y = (ray.voxel_location.position.int_component.y & (~sublocation_mask)) | (y_sublocation.int_component & sublocation_mask);
  ray.voxel_location.position.dec_component.y = y_sublocation.dec_component;
  ray.voxel_location.position.int_component.z = (ray.voxel_location.position.int_component.z & (~sublocation_mask)) | (z_sublocation.int_component & sublocation_mask);
  ray.voxel_location.position.dec_component.z = z_sublocation.dec_component;

  return neighbor;
}


bool jumpToNeighbor(inout Ray ray, uint neighbor)
{
  // return value: true if the ray is within bounds of the world, false otherwise
  uint max_location = uint(0xFFFFFFFF) >> (32-octree_layers+1);
  ray.voxel_location.type = 0;
  if (neighbor == 0)
  {
    if (ray.voxel_location.position.int_component.x == 0) return false;
    ray.voxel_location.position.int_component.x--;
    ray.voxel_location.position.dec_component.x = 1.0;
  }
  else if (neighbor == 1)
  {
    if (ray.voxel_location.position.int_component.x == max_location) return false;
    ray.voxel_location.position.int_component.x++;
    ray.voxel_location.position.dec_component.x = 0.0;
  }
  else if (neighbor == 2)
  {
    if (ray.voxel_location.position.int_component.y == 0) return false;
    ray.voxel_location.position.int_component.y--;
    ray.voxel_location.position.dec_component.y = 1.0;
  }
  else if (neighbor == 3)
  {
    if (ray.voxel_location.position.int_component.y == max_location) return false;
    ray.voxel_location.position.int_component.y++;
    ray.voxel_location.position.dec_component.y = 0.0;
  }
  else if (neighbor == 4)
  {
    if (ray.voxel_location.position.int_component.z == 0) return false;
    ray.voxel_location.position.int_component.z--;
    ray.voxel_location.position.dec_component.z = 1.0;
  }
  else if (neighbor == 5)
  {
    if (ray.voxel_location.position.int_component.z == max_location) return false;
    ray.voxel_location.position.int_component.z++;
    ray.voxel_location.position.dec_component.z = 0.0;
  }


  uvec3 int_position = ray.voxel_location.position.int_component;

  ray.voxel_location.layer = octree_layers;
  uint current_octree_index = 0;
  bool found_uniform = false;
  for (uint i = 0; i < octree_layers-1; i++)
  {
    uint shifter = octree_layers-i-2;
    uint current_octant = ((int_position.x >> shifter) & 1u) | (((int_position.y >> shifter) & 1u) << 2) | (((~int_position.z >> shifter) & 1u) << 1);

    current_octree_index = indirection_pool[(current_octree_index<<3) | current_octant];

    ray.voxel_location.layer--;

    ray.voxel_location.type = voxel_type_pool[current_octree_index];
    if (ray.voxel_location.type != 0)
    {
      found_uniform = true;
      break;
    }

    if (current_octree_index == 0)
    {
      found_uniform = true;
      break;
    }
  }
  if (!found_uniform)
  {
    ray.voxel_location.type = 1;
  }
  return true;
}


bool rayMarch(inout Ray ray)
{
  uint neighbor = stepToEdge(ray);
  bool is_within_bounds = jumpToNeighbor(ray, neighbor);
  ray.num_steps++;
  if (!is_within_bounds)
  {
    ray.voxel_location.type = 0;
    return true;
  }
  return ray.voxel_location.type != 0;
}


uinfl uinflAdd(uinfl first, uinfl second)
{
  uinfl result;
  result.dec_component = first.dec_component + second.dec_component;
  result.int_component = uint(result.dec_component);
  result.dec_component -= result.int_component;
  result.int_component += first.int_component + second.int_component;
  return result;
}
uinfl uinflSub(uinfl first, uinfl second)
{
  first.int_component -= second.int_component;
  first.dec_component -= second.dec_component;
  if (first.dec_component < 0.0)
  {
    first.dec_component += 1.0;
    first.int_component -= 1;
  }
  return first;
}

ufvec3 uf3Add(ufvec3 first, ufvec3 second)
{
  ufvec3 result;
  result.dec_component = first.dec_component + second.dec_component;
  result.int_component = uvec3(result.dec_component);
  result.dec_component -= result.int_component;
  result.int_component += first.int_component + second.int_component;
  return result;
}
ufvec3 uf3Sub(ufvec3 first, ufvec3 second)
{
  first.int_component -= second.int_component;
  first.dec_component -= second.dec_component;
  if (first.dec_component.x < 0.0)
  {
    first.dec_component.x += 1.0;
    first.int_component.x -= 1;
  }
  if (first.dec_component.y < 0.0)
  {
    first.dec_component.y += 1.0;
    first.int_component.y -= 1;
  }
  if (first.dec_component.z < 0.0)
  {
    first.dec_component.z += 1.0;
    first.int_component.z -= 1;
  }
  return first;
}
