cmake_minimum_required(VERSION 3.16)

# 该子项目不支持独立构建，必须通过 NanoAI_RKNN 根 CMake 选择构建。
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR
        "projects/test_project does not support standalone configure. "
        "Configure from NanoAI_RKNN root with -DNANOAI_RKNN_BUILD_MODE=PROJECTS and select this project. "
        "Cross-compiler and tool paths should be provided from the root configure/build invocation."
    )
endif()

# ======================================================
# NanoAI RKNN 模块来源
# ======================================================
set(NANOAI_RKNN_TARGET NanoAI_RKNN)

if(TARGET NanoAI_RKNN::NanoAI_RKNN)
    set(NANOAI_RKNN_TARGET NanoAI_RKNN::NanoAI_RKNN)
elseif(TARGET NanoAI_RKNN)
    set(NANOAI_RKNN_TARGET NanoAI_RKNN)
else()
    message(FATAL_ERROR
        "NanoAI_RKNN target not found. This subproject must be built from NanoAI_RKNN root."
    )
endif()

set(NANOAI_RKNN_DEMO_TARGET NanoAI_rknn_demo)

add_executable(${NANOAI_RKNN_DEMO_TARGET}
    ${CMAKE_CURRENT_LIST_DIR}/demo_main.cpp
)

target_link_libraries(${NANOAI_RKNN_DEMO_TARGET}
    PRIVATE
    ${NANOAI_RKNN_TARGET}
)