set(INCLUDE_DIRS ${INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)

set(HEADERS ${HEADERS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include/anthrax.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/camera.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/character.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/idmap.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/intfloat.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/material.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/octree.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/text.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/world.hpp
	PARENT_SCOPE
  )

set(SRC ${SRC}
  ${CMAKE_CURRENT_SOURCE_DIR}/src/anthrax.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/intfloat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/material.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/octree.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/world.cpp
	PARENT_SCOPE
  )
