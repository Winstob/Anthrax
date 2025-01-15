set(INCLUDE_DIRS ${INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)

set(HEADERS ${HEADERS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include/tools.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/ringbuffer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/quaternion.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/timer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/freelist.hpp
	PARENT_SCOPE
  )

set(SRC ${SRC}
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tools.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/quaternion.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/timer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/freelist.cpp
	PARENT_SCOPE
  )
