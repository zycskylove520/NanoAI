include_guard(GLOBAL)

# opencv
set(NANOAI_RKNN_OPENCV_DIR "" CACHE PATH "OpenCV_DIR for find_package(OpenCV)")
if(NOT CMAKE_CROSSCOMPILING)
    message(STATUS "Skip OpenCV dependency checks during native configure probe.")
    return()
endif()

if(NANOAI_RKNN_OPENCV_DIR STREQUAL "")
    message(FATAL_ERROR
        "OpenCV dependency is enabled but OpenCV_DIR is not set. "
        "Please provide -DNANOAI_RKNN_OPENCV_DIR=/path/to/opencv4/cmake directory."
    )
endif()

set(OpenCV_DIR ${NANOAI_RKNN_OPENCV_DIR})

find_package(OpenCV REQUIRED)
list(APPEND NANOAI_RKNN_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS})
list(APPEND NANOAI_RKNN_LINK_LIBS ${OpenCV_LIBS})
