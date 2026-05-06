include_guard(GLOBAL)

# stb_image
set(NANOAI_RKNN_STB_IMAGE_INCLUDE_DIR "" CACHE PATH "stb_image include directory")
if(NOT CMAKE_CROSSCOMPILING)
	message(STATUS "Skip stb_image dependency checks during native configure probe.")
	return()
endif()

if(NANOAI_RKNN_STB_IMAGE_INCLUDE_DIR STREQUAL "")
	message(FATAL_ERROR
		"stb_image dependency is enabled but include path is not set. "
		"Please provide -DNANOAI_RKNN_STB_IMAGE_INCLUDE_DIR=/path/to/stb_image."
	)
endif()

set(STB_IMAGE_INCLUDE_DIR ${NANOAI_RKNN_STB_IMAGE_INCLUDE_DIR})
list(APPEND NANOAI_RKNN_INCLUDE_DIRS ${STB_IMAGE_INCLUDE_DIR})
