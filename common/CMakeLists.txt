set(INCLUDE_DIRS ${INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)

set(HEADERS ${HEADERS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include/ringbuffer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/quaternion.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/timer.hpp
	PARENT_SCOPE
  )

set(SRC ${SRC}
  ${CMAKE_CURRENT_SOURCE_DIR}/src/quaternion.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/timer.cpp
	PARENT_SCOPE
  )
