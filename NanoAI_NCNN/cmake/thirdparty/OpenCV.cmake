include_guard(GLOBAL)

set(_NANOAI_NCNN_DEFAULT_OPENCV_DIR "")

set(NANOAI_NCNN_OPENCV_DIR "${_NANOAI_NCNN_DEFAULT_OPENCV_DIR}" CACHE PATH "OpenCV_DIR for find_package(OpenCV)")

if(NANOAI_NCNN_WITH_OPENCV)
    if(NANOAI_NCNN_OPENCV_DIR STREQUAL "")
        message(FATAL_ERROR
            "OpenCV dependency is enabled but OpenCV_DIR is not set. "
            "Please provide -DNANOAI_NCNN_OPENCV_DIR=/path/to/OpenCV/sdk/native/jni."
        )
    endif()

    set(OpenCV_DIR ${NANOAI_NCNN_OPENCV_DIR})

    find_package(OpenCV REQUIRED)
    list(APPEND NANOAI_NCNN_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS})
    list(APPEND NANOAI_NCNN_LINK_LIBS ${OpenCV_LIBS})
endif()
