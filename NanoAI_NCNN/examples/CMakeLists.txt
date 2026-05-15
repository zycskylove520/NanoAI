# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: CMakeLists.txt
# Brief: 聚合 NanoAI_NCNN 示例子目录，并根据顶层选择变量决定是否参与构建。
#
# Notes:
# - 示例发现逻辑依赖子目录中存在 `CMakeLists.txt`，新增示例时保持该约定即可自动纳入。
# - 这里的开关只影响 examples 子目录，不改变主库 `NanoAI_NCNN` 的导出与安装行为。

# 统一扫描直接子目录下的示例工程，避免顶层脚本手工维护示例列表。

file(GLOB _NANOAI_NCNN_EXAMPLE_CMAKES RELATIVE ${CMAKE_CURRENT_LIST_DIR} "*/CMakeLists.txt")
set(_NANOAI_NCNN_AVAILABLE_EXAMPLES)
foreach(_example_cmake IN LISTS _NANOAI_NCNN_EXAMPLE_CMAKES)
    get_filename_component(_example_name "${_example_cmake}" DIRECTORY)
    list(APPEND _NANOAI_NCNN_AVAILABLE_EXAMPLES ${_example_name})
endforeach()

set(_NANOAI_NCNN_SELECTED_EXAMPLES)
if(NANOAI_NCNN_EXAMPLES STREQUAL "ALL")
    set(_NANOAI_NCNN_SELECTED_EXAMPLES ${_NANOAI_NCNN_AVAILABLE_EXAMPLES})
else()
    if(NANOAI_NCNN_EXAMPLES STREQUAL "NONE")
        # 与“关闭全部示例”相比，顶层已经有 `NANOAI_NCNN_BUILD_EXAMPLES` 控制总开关，
        # 因此这里禁止 NONE，避免两个层级的语义重复且容易误配。
        message(FATAL_ERROR
            "NANOAI_NCNN_EXAMPLES='NONE' is not allowed. "
            "Use ALL or a semicolon-separated list of example names."
        )
    endif()

    set(_NANOAI_NCNN_SELECTED_EXAMPLES ${NANOAI_NCNN_EXAMPLES})

    foreach(_selected_example IN LISTS _NANOAI_NCNN_SELECTED_EXAMPLES)
        list(FIND _NANOAI_NCNN_AVAILABLE_EXAMPLES ${_selected_example} _selected_index)
        if(_selected_index LESS 0)
            message(FATAL_ERROR
                "Unknown example in NANOAI_NCNN_EXAMPLES: '${_selected_example}'. "
                "Available examples: ${_NANOAI_NCNN_AVAILABLE_EXAMPLES}"
            )
        endif()
    endforeach()
endif()

foreach(_example_name IN LISTS _NANOAI_NCNN_AVAILABLE_EXAMPLES)
    string(TOUPPER ${_example_name} _example_name_upper)
    set(_example_option_name NANOAI_ENABLE_EXAMPLE_${_example_name_upper})
    option(${_example_option_name} "Build ${_example_name} example" OFF)

    if(NANOAI_NCNN_EXAMPLES STREQUAL "ALL")
        set(${_example_option_name} ON CACHE BOOL "Build ${_example_name} example" FORCE)
    else()
        # 为每个示例生成独立缓存开关，方便 IDE 用户在 configure 后直观看到最终选择结果。
        list(FIND _NANOAI_NCNN_SELECTED_EXAMPLES ${_example_name} _selected_index)
        if(_selected_index GREATER -1)
            set(${_example_option_name} ON CACHE BOOL "Build ${_example_name} example" FORCE)
        else()
            set(${_example_option_name} OFF CACHE BOOL "Build ${_example_name} example" FORCE)
        endif()
    endif()

    if(${_example_option_name})
        # 仅把被选中的示例加入工程，减少不必要的依赖探测与编译时间。
        add_subdirectory(${_example_name})
    endif()
endforeach()
