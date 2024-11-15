set(INCLUDE_DIRS ${INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)

set(HEADERS ${HEADERS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include/transform.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/mesh.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/voxelizer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vox_handler.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/gltf_handler.hpp
	PARENT_SCOPE
  )

set(SRC ${SRC}
  ${CMAKE_CURRENT_SOURCE_DIR}/src/transform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mesh.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/voxelizer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/vox_handler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gltf_handler.cpp
	PARENT_SCOPE
  )
