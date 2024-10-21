project(anthrax)

set(PROJECT_NAME anthrax)

set(COMPILE_FLAGS "-g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")

#add_compile_definitions(SHADER_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}/shaders)
add_compile_definitions(SHADER_DIRECTORY=shaders/)

set(GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_LIBRARY_TYPE STATIC)

find_package(Vulkan REQUIRED)

set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/glad)

set(GLM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/glm)

set(FREETYPE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/freetype)

add_subdirectory(${GLFW_DIR})
add_subdirectory(${GLAD_DIR})
add_subdirectory(${GLM_DIR})
add_subdirectory(${FREETYPE_DIR})

set(TOOLS
  ${CMAKE_CURRENT_SOURCE_DIR}/tools/tools.hpp
  )

set(HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/anthrax.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/buffer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/camera.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/character.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compute_shader_manager.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/descriptor.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/device.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/graphics_pipeline.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/idmap.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/image.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/input_handler.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/intfloat.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/material.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/octree.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/render_pass.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/shader.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/swap_chain.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/text.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan_manager.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/world.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/voxfile_parser.hpp
  )

set(SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/anthrax.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/compute_shader_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics_pipeline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/image.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/input_handler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/intfloat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/material.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/octree.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/render_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/shader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/swap_chain.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/world.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/voxfile_parser.cpp
  )

#set(SHADERS
#  ${CMAKE_CURRENT_SOURCE_DIR}/include/internalshaders.hpp
#  )

add_library(${PROJECT_NAME}
  ${TOOLS}
  ${SRC}
  ${HEADERS}
  )

target_link_libraries(${PROJECT_NAME}
  glfw
  glad
  glm
  freetype
  vulkan
  )

target_include_directories(${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/tools
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
