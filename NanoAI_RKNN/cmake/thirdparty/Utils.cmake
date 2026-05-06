include_guard(GLOBAL)

# utils
set(NANOAI_RKNN_UTILS_ROOT "" CACHE PATH "RKNN utils root directory")
if(NOT CMAKE_CROSSCOMPILING)
	message(STATUS "Skip utils dependency checks during native configure probe.")
	return()
endif()

if(NANOAI_RKNN_UTILS_ROOT STREQUAL "")
	message(FATAL_ERROR
		"utils dependency is enabled but root path is not set. "
		"Please provide -DNANOAI_RKNN_UTILS_ROOT=/path/to/utils."
	)
endif()

set(UTILS_DIR ${NANOAI_RKNN_UTILS_ROOT})
set(UTILS_INCLUDE_DIR ${UTILS_DIR}/include)
set(UTILS_SOURCE_DIR ${UTILS_DIR}/src)
list(APPEND NANOAI_RKNN_INCLUDE_DIRS ${UTILS_INCLUDE_DIR})
file(GLOB_RECURSE UTILS_SOURCES ${UTILS_SOURCE_DIR}/*.c ${UTILS_SOURCE_DIR}/*.cpp)
list(APPEND NANOAI_RKNN_SRCS ${UTILS_SOURCES})
