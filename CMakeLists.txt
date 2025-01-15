project(anthrax)

set(PROJECT_NAME anthrax)

set(COMPILE_FLAGS "-g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules;${CMAKE_MODULE_PATH})

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
set(JSONCPP_DIR ${LIB_DIR}/jsoncpp)
set(STB_DIR ${LIB_DIR}/stb)

add_subdirectory(${GLFW_DIR})
add_subdirectory(${GLAD_DIR})
add_subdirectory(${GLM_DIR})
add_subdirectory(${FREETYPE_DIR})
add_subdirectory(${JSONCPP_DIR})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/common)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/main)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/model_handler)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vulkan_handler)

add_library(${PROJECT_NAME}
  ${SRC}
  ${HEADERS}
  )

target_link_libraries(${PROJECT_NAME}
  glfw
  glad
  glm
  freetype
	jsoncpp
  vulkan
  )

target_include_directories(${PROJECT_NAME}
  PUBLIC
	${JSONCPP_DIR}/include
	${STB_DIR}
	${INCLUDE_DIRS}
  )
