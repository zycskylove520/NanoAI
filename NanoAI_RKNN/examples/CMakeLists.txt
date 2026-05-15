# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: CMakeLists.txt
# Brief: 统一发现并按配置选择 `examples/` 下的 RKNN 示例子目录。
#
# Notes:
# - 该文件只负责示例选择与分发，不直接定义具体示例目标。
# - 示例名校验集中在这里完成，便于在根配置阶段尽早暴露拼写错误。

# Aggregated examples entry.
# 自动扫描一级子目录中的 `CMakeLists.txt`，约定每个示例独占一个目录。

file(GLOB _NANOAI_RKNN_EXAMPLE_CMAKES RELATIVE ${CMAKE_CURRENT_LIST_DIR} "*/CMakeLists.txt")
set(_NANOAI_RKNN_AVAILABLE_EXAMPLES)
foreach(_example_cmake IN LISTS _NANOAI_RKNN_EXAMPLE_CMAKES)
    get_filename_component(_example_name "${_example_cmake}" DIRECTORY)
    list(APPEND _NANOAI_RKNN_AVAILABLE_EXAMPLES ${_example_name})
endforeach()

set(_NANOAI_RKNN_SELECTED_EXAMPLES)
if(NANOAI_RKNN_EXAMPLES STREQUAL "ALL")
    set(_NANOAI_RKNN_SELECTED_EXAMPLES ${_NANOAI_RKNN_AVAILABLE_EXAMPLES})
else()
    if(NANOAI_RKNN_EXAMPLES STREQUAL "NONE")
        # 即使不想构建示例，也应通过顶层 `NANOAI_RKNN_BUILD_EXAMPLES=OFF` 表达，
        # 避免 `examples/` 已进入后又出现语义重复的“空选择”。
        message(FATAL_ERROR
            "NANOAI_RKNN_EXAMPLES='NONE' is not allowed. "
            "Use ALL or a semicolon-separated list of example names."
        )
    endif()

    set(_NANOAI_RKNN_SELECTED_EXAMPLES ${NANOAI_RKNN_EXAMPLES})

    foreach(_selected_example IN LISTS _NANOAI_RKNN_SELECTED_EXAMPLES)
        list(FIND _NANOAI_RKNN_AVAILABLE_EXAMPLES ${_selected_example} _selected_index)
        if(_selected_index LESS 0)
            # 在配置阶段立即报错，避免用户以为示例已被选中但最终什么都没编译。
            message(FATAL_ERROR
                "Unknown example in NANOAI_RKNN_EXAMPLES: '${_selected_example}'. "
                "Available examples: ${_NANOAI_RKNN_AVAILABLE_EXAMPLES}"
            )
        endif()
    endforeach()
endif()

foreach(_example_name IN LISTS _NANOAI_RKNN_AVAILABLE_EXAMPLES)
    string(TOUPPER ${_example_name} _example_name_upper)
    set(_example_option_name NANOAI_ENABLE_EXAMPLE_${_example_name_upper})
    # 为每个示例生成独立 cache 开关，便于 IDE 或高级用户在图形界面中查看最终选择结果。
    option(${_example_option_name} "Build ${_example_name} example" OFF)

    if(NANOAI_RKNN_EXAMPLES STREQUAL "ALL")
        set(${_example_option_name} ON CACHE BOOL "Build ${_example_name} example" FORCE)
    else()
        list(FIND _NANOAI_RKNN_SELECTED_EXAMPLES ${_example_name} _selected_index)
        if(_selected_index GREATER -1)
            set(${_example_option_name} ON CACHE BOOL "Build ${_example_name} example" FORCE)
        else()
            set(${_example_option_name} OFF CACHE BOOL "Build ${_example_name} example" FORCE)
        endif()
    endif()

    if(${_example_option_name})
        # 只有显式启用的示例才进入 add_subdirectory，避免无关子目录触发额外依赖检查。
        add_subdirectory(${_example_name})
    endif()
endforeach()
