project(anthrax)

set(PROJECT_NAME anthrax)

add_compile_definitions(SHADER_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}/shaders)

set(GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_LIBRARY_TYPE STATIC)

set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/glad)

set(GLM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/glm)

add_subdirectory(${GLFW_DIR})
add_subdirectory(${GLAD_DIR})
add_subdirectory(${GLM_DIR})

set(HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/anthrax.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/shader.hpp
  )

set(SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/anthrax.cpp
  )

#set(SHADERS
#  ${CMAKE_CURRENT_SOURCE_DIR}/include/internalshaders.hpp
#  )

add_library(${PROJECT_NAME}
  ${SRC}
  ${HEADERS}
  )

target_link_libraries(${PROJECT_NAME}
  glfw
  glad
  glm
  )

target_include_directories(${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
