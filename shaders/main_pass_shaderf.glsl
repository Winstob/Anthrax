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

struct ufvec3
{
  uvec3 int_component;
  vec3 dec_component;
};

struct DirectedLight
{
  vec3 direction;
  vec3 scatter_color;
  vec3 color;
};

struct Material
{
  vec3 diffuse;
  vec3 specular;
  float shininess;
  float opacity;
};

uniform int octree_layers;
uniform float focal_distance;
uniform int screen_width;
uniform int screen_height;
uniform WorldLocation camera_position;
uniform vec3 camera_right;
uniform vec3 camera_up;
uniform vec3 camera_forward;
uniform DirectedLight sunlight;

struct Voxel
{
  ufvec3 position; // The position of the voxel where the origin is at the corner of the world (not the center)
  uint layer;
  uint type;
};

struct Ray
{
  vec3 ray_dir;
  Voxel voxel_location;
  uint num_steps;
  float distance_traveled;
  vec3 surface_normal;
  vec4 color;
};

const float render_distance = 100.0 * 1000 * 10; // 10KM render distance
//const float render_distance = 1024.0; // 64 chunk render distance
const uint lod_multiplier = 16;


vec3 calculateMainRayDirection();
Voxel getVoxelInfo(in ufvec3 world_position, in uint lod_layer);
Voxel findVoxelLocation(WorldLocation world_location);
double findSafeDistance(in Ray ray);
bool oldRayMarch(inout Ray ray);
bool rayMarch(inout Ray ray);
uint computeLOD(float dist);

Material materialLookup(uint id);
float lerp(float a, float b, float f);
void addSunlight(inout Ray incoming_ray, in Material material);
void addAmbientOcclusion(inout Ray incoming_ray, in Material material);

void main()
{
  Ray ray;
  ray.ray_dir = calculateMainRayDirection();
  ray.voxel_location = findVoxelLocation(camera_position);
  ray.num_steps = 0;
  ray.distance_traveled = 0.0;
  ray.surface_normal = vec3(0.0, 0.0, 0.0);
  ray.color = vec4(0.0, 0.0, 0.0, 0.0);

  uint voxel_type;
  for (uint i = 0; i < pow(2, octree_layers); i++)
  {
    //if (rayMarch(ray) || ray.distance_traveled >= render_distance)
    if (rayMarch(ray))
    {
      break;
    }
  }
  voxel_type = ray.voxel_location.type;

  //uint voxel_type = indirection_pool[3];
  if (voxel_type == 0)
  {
    FragColor = vec4(0.0, 0.0, 0.0, 0.0);
  }
  else
  {
    //FragColor = vec4(ray.voxel_location.position.dec_component, 1.0);
    //FragColor = vec4(1.0-vec3(ray.distance_traveled/(1<<(octree_layers+1))), 1.0);

    Material material = materialLookup(ray.voxel_location.type);
    //ray.color = vec4(material.diffuse, material.opacity);
    addSunlight(ray, material);
    addAmbientOcclusion(ray, material);
    FragColor = ray.color;
  }
  //FragColor.xyz += vec3(ray.distance_traveled / render_distance);
  //FragColor = vec4(ray.voxel_location.position.dec_component, 1.0);
  //FragColor = vec4(vec3(ray.distance_traveled / pow(2, octree_layers)), 1.0);
  //FragColor = vec4(vec3(float(ray.num_steps) / 32.0), 1.0);


  //FragColor = FragColor * max((1-ray.distance_traveled/render_distance), 0.0); // Fog
}


vec3 calculateMainRayDirection()
{
  vec3 x = camera_right * screen_position.x;
  vec3 y = camera_up * screen_position.y * float(screen_height) / float(screen_width);
  vec3 z = camera_forward * focal_distance;
  return normalize(x + y + z);
}


Voxel getVoxelInfo(in ufvec3 world_position, in uint lod_layer)
{
  Voxel voxel;
  voxel.position = world_position;

  voxel.layer = octree_layers;
  uint current_octree_index = 0;
  bool found_uniform = false;
  for (uint i = 0; i < octree_layers-lod_layer; i++)
  {
    uint shifter = octree_layers-i-2;
    uint current_octant = ((world_position.int_component.x >> shifter) & 1u) | (((world_position.int_component.y >> shifter) & 1u) << 2) | (((~world_position.int_component.z >> shifter) & 1u) << 1);

    current_octree_index = indirection_pool[(current_octree_index<<3) | current_octant];

    voxel.layer--;

    voxel.type = voxel_type_pool[current_octree_index];
    if (voxel.type != 0)
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
    // This is either a recursive element (fractal) or the LOD limit has been reached.
    voxel.type = 1;
  }

  return voxel;

}


Voxel findVoxelLocation(WorldLocation world_location)
{
  Voxel voxel_location;

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

  voxel_location = getVoxelInfo(voxel_location.position, 1);

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

    ray.distance_traveled += float(x_factor);
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

    ray.distance_traveled += float(y_factor);
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

    ray.distance_traveled += float(z_factor);
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
    ray.surface_normal = vec3(1.0, 0.0, 0.0);
    ray.voxel_location.position.int_component.x--;
    ray.voxel_location.position.dec_component.x = 1.0;
  }
  else if (neighbor == 1)
  {
    if (ray.voxel_location.position.int_component.x == max_location) return false;
    ray.surface_normal = vec3(-1.0, 0.0, 0.0);
    ray.voxel_location.position.int_component.x++;
    ray.voxel_location.position.dec_component.x = 0.0;
  }
  else if (neighbor == 2)
  {
    if (ray.voxel_location.position.int_component.y == 0) return false;
    ray.surface_normal = vec3(0.0, 1.0, 0.0);
    ray.voxel_location.position.int_component.y--;
    ray.voxel_location.position.dec_component.y = 1.0;
  }
  else if (neighbor == 3)
  {
    if (ray.voxel_location.position.int_component.y == max_location) return false;
    ray.surface_normal = vec3(0.0, -1.0, 0.0);
    ray.voxel_location.position.int_component.y++;
    ray.voxel_location.position.dec_component.y = 0.0;
  }
  else if (neighbor == 4)
  {
    if (ray.voxel_location.position.int_component.z == 0) return false;
    ray.surface_normal = vec3(0.0, 0.0, 1.0);
    ray.voxel_location.position.int_component.z--;
    ray.voxel_location.position.dec_component.z = 1.0;
  }
  else if (neighbor == 5)
  {
    if (ray.voxel_location.position.int_component.z == max_location) return false;
    ray.surface_normal = vec3(0.0, 0.0, -1.0);
    ray.voxel_location.position.int_component.z++;
    ray.voxel_location.position.dec_component.z = 0.0;
  }

  ray.voxel_location = getVoxelInfo(ray.voxel_location.position, computeLOD(ray.distance_traveled));

  return true;
}


bool rayMarch(inout Ray ray)
{
  // Marches a ray forwards to the boundary of the neighboring voxel
  // Returns true if the ray has hit a non-air voxel or has gone out of bounds, false otherwise
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


uint computeLOD(float dist)
{
  // Compute the lowest reasonable octree layer to use based on the given distance
  uint num_pixels = max(screen_width, screen_height);
  uint num_visible_pixels = uint(dist / focal_distance)*lod_multiplier;
  return max(uint(log2(num_visible_pixels / num_pixels)), 1);
}


Material materialLookup(uint id)
{
  // TODO: make a material lookup table defined by CPU
  Material material;
  if (id == 1)
  {
    material.diffuse = vec3(0.2, 0.3, 0.9);
    material.specular = vec3(0.2, 0.2, 0.2);
    material.shininess = 32.0f;
    material.opacity = 1.0;
  }

  return material;
}


float lerp(float a, float b, float f)
{
  return a + f * (b - a);
}


void addSunlight(inout Ray incoming_ray, in Material material)
{
  Ray ray = incoming_ray;
  ray.ray_dir = -sunlight.direction;
  bool is_in_shadow = false;
  if (dot(incoming_ray.surface_normal, ray.ray_dir) <= 0.0)
  {
    is_in_shadow = true;
  }
  else
  {
    for (uint i = 0; i < pow(2, octree_layers); i++)
    {
      if (rayMarch(ray))
      {
        break;
      }
    }
    if (ray.voxel_location.type != 0)
    {
      // This voxel is not exposed to sunlight
      is_in_shadow = true;
    }
  }

  vec3 color = material.diffuse * sunlight.scatter_color;
  if (!is_in_shadow)
  {
    float diffuse_factor = max(dot(-sunlight.direction, incoming_ray.surface_normal), 0.0);
    color += diffuse_factor * material.diffuse * sunlight.scatter_color;

    vec3 halfway_direction = normalize(normalize(-sunlight.direction) + -incoming_ray.ray_dir);
    float spec = pow(max(dot(incoming_ray.surface_normal, halfway_direction), 0.0), material.shininess*256.0f);
    color += sunlight.color * (spec * material.specular);
  }

  incoming_ray.color += vec4(color*material.opacity, material.opacity);

  return;
}


void addAmbientOcclusion(inout Ray incoming_ray, in Material material)
{
  float ao_component = 1.0;

  float ao_distance = 1.0f;
  //uint num_rays = (2*uint(ceil(ao_distance)))*4;
  uint num_rays = 8;
  Ray ray;
  float x_component = 0.0;
  float z_component = 1.0;
  float x_increment = 4.0 / num_rays;
  float z_increment = 2.0 / num_rays;

  mat3 rotation = mat3(1.0-abs(incoming_ray.surface_normal.x), incoming_ray.surface_normal.x, 0.0,
       abs(incoming_ray.surface_normal.x), incoming_ray.surface_normal.y, abs(incoming_ray.surface_normal.z),
       0.0, incoming_ray.surface_normal.z, 1.0-abs(incoming_ray.surface_normal.z));

  for (uint i = 0; i < num_rays; i++)
  {
    ray = incoming_ray;
    ray.num_steps = 0;
    ray.distance_traveled = 0.0;
    ray.ray_dir.x = x_component-1.0;
    ray.ray_dir.y = ao_distance;
    ray.ray_dir.z = z_component-1.0;
    x_component = (mod(x_component+x_increment, 2.0));
    z_component = (mod(z_component+z_increment, 2.0));

    ray.ray_dir = normalize(ray.ray_dir) * rotation;

    rayMarch(ray);
    ray.distance_traveled = 0.0;
    for (uint j = 0; j < ceil(ao_distance); j++)
    {
      if (!rayMarch(ray) || ray.distance_traveled > ao_distance)
        break;
    }
    if (ray.distance_traveled > ao_distance)
      ray.voxel_location.type = 0;
    if (ray.voxel_location.type != 0)
    {
      Material material = materialLookup(ray.voxel_location.type);
      float normalized_distance = ray.distance_traveled/ao_distance;
      ao_component -= (lerp(1.0, 0.01, normalized_distance) * material.opacity) / float(num_rays);
      //ao_component -= ray.distance_traveled/ao_distance * material.opacity / float(num_rays);
    }
  }

  incoming_ray.color.xyz *= ao_component;
  return;
}
