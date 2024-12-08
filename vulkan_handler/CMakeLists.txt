set(INCLUDE_DIRS ${INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)

set(HEADERS ${HEADERS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include/buffer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compute_shader_manager.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/descriptor.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/device.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/graphics_pipeline.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/image.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/input_handler.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/render_pass.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/shader.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/swap_chain.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan_manager.hpp
	PARENT_SCOPE
  )

set(SRC ${SRC}
  ${CMAKE_CURRENT_SOURCE_DIR}/src/buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/compute_shader_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics_pipeline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/image.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/input_handler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/render_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/shader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/swap_chain.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_manager.cpp
	PARENT_SCOPE
  )
