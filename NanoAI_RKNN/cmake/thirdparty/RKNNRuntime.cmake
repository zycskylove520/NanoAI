include_guard(GLOBAL)

# rknn_runtime (external path required)
set(NANOAI_RKNN_RUNTIME_INCLUDE_DIR "" CACHE PATH "RKNN runtime include directory")
set(NANOAI_RKNN_RUNTIME_LIBRARY_DIR "" CACHE PATH "RKNN runtime library directory")

if(NOT CMAKE_CROSSCOMPILING)
	message(STATUS "Skip RKNN runtime dependency checks during native configure probe.")
	return()
endif()

if(NANOAI_RKNN_RUNTIME_INCLUDE_DIR STREQUAL "" OR NANOAI_RKNN_RUNTIME_LIBRARY_DIR STREQUAL "")
	message(FATAL_ERROR
		"RKNN runtime dependency is enabled but paths are not set. "
		"Please provide -DNANOAI_RKNN_RUNTIME_INCLUDE_DIR and -DNANOAI_RKNN_RUNTIME_LIBRARY_DIR."
	)
endif()

set(RKNN_RT_INCLUDE_DIR ${NANOAI_RKNN_RUNTIME_INCLUDE_DIR})
set(RKNN_RT_LIB_DIR ${NANOAI_RKNN_RUNTIME_LIBRARY_DIR})
list(APPEND NANOAI_RKNN_INCLUDE_DIRS ${RKNN_RT_INCLUDE_DIR})
list(APPEND NANOAI_RKNN_LINK_DIRS ${RKNN_RT_LIB_DIR})
list(APPEND NANOAI_RKNN_LINK_LIBS rknnrt)
