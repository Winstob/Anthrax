project(anthrax)

set(PROJECT_NAME anthrax)

set(COMPILE_FLAGS "-g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")

#add_compile_definitions(SHADER_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}/shaders)
add_compile_definitions(SHADER_DIRECTORY=shaders/)

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

set(GLFW_DIR ${LIB_DIR}/glfw)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_LIBRARY_TYPE STATIC)

find_package(Vulkan REQUIRED)

set(GLAD_DIR ${LIB_DIR}/glad)

set(GLM_DIR ${LIB_DIR}/glm)

set(FREETYPE_DIR ${LIB_DIR}/freetype)

add_subdirectory(${GLFW_DIR})
add_subdirectory(${GLAD_DIR})
add_subdirectory(${GLM_DIR})
add_subdirectory(${FREETYPE_DIR})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/main)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/model_handler)

set(TOOLS
  ${CMAKE_CURRENT_SOURCE_DIR}/tools/tools.hpp
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
	${INCLUDE_DIRS}
  )
