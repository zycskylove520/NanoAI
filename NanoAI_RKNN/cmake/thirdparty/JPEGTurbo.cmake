include_guard(GLOBAL)

# jpeg_turbo
set(NANOAI_RKNN_JPEG_TURBO_ROOT "" CACHE PATH "jpeg_turbo root directory")
if(NOT CMAKE_CROSSCOMPILING)
	message(STATUS "Skip jpeg_turbo dependency checks during native configure probe.")
	return()
endif()

if(NANOAI_RKNN_JPEG_TURBO_ROOT STREQUAL "")
	message(FATAL_ERROR
		"jpeg_turbo dependency is enabled but root path is not set. "
		"Please provide -DNANOAI_RKNN_JPEG_TURBO_ROOT=/path/to/jpeg_turbo."
	)
endif()

set(JPEG_TURBO_DIR ${NANOAI_RKNN_JPEG_TURBO_ROOT})
set(JPEG_TURBO_INCLUDE_DIR ${JPEG_TURBO_DIR}/include)
set(JPEG_TURBO_LIB_DIR ${JPEG_TURBO_DIR}/Linux/aarch64)
list(APPEND NANOAI_RKNN_INCLUDE_DIRS ${JPEG_TURBO_INCLUDE_DIR})
list(APPEND NANOAI_RKNN_LINK_DIRS ${JPEG_TURBO_LIB_DIR})
list(APPEND NANOAI_RKNN_LINK_LIBS turbojpeg)
