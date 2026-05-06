include_guard(GLOBAL)

# rga (external path required)
set(NANOAI_RKNN_RGA_ROOT "" CACHE PATH "RGA root directory")
if(NOT CMAKE_CROSSCOMPILING)
    message(STATUS "Skip RGA dependency checks during native configure probe.")
    return()
endif()

if(NANOAI_RKNN_RGA_ROOT STREQUAL "")
    message(FATAL_ERROR
        "RGA dependency is enabled but root path is not set. "
        "Please provide -DNANOAI_RKNN_RGA_ROOT=/path/to/librga."
    )
endif()

set(RGA_DIR ${NANOAI_RKNN_RGA_ROOT})
set(RGA_INCLUDE_DIR ${RGA_DIR}/include)
set(RGA_LIB_DIR ${RGA_DIR}/libs/Linux/gcc-aarch64)
list(APPEND NANOAI_RKNN_INCLUDE_DIRS ${RGA_INCLUDE_DIR} ${RGA_DIR}/utils/allocator/include)
list(APPEND NANOAI_RKNN_LINK_DIRS ${RGA_LIB_DIR})
list(APPEND NANOAI_RKNN_LINK_LIBS rga)

# rga utils
file(GLOB_RECURSE RGA_UTILS_SRCS
    ${RGA_DIR}/utils/src/*.cpp
    ${RGA_DIR}/utils/allocator/src/*.cpp
)
list(APPEND NANOAI_RKNN_SRCS ${RGA_UTILS_SRCS})
