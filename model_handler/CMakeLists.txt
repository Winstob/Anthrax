set(INCLUDE_DIRS ${INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)

set(HEADERS ${HEADERS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vox_handler.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/glb_handler.hpp
	PARENT_SCOPE
  )

set(SRC ${SRC}
  ${CMAKE_CURRENT_SOURCE_DIR}/src/vox_handler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/glb_handler.cpp
	PARENT_SCOPE
  )
