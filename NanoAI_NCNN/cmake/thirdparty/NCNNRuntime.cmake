# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: NCNNRuntime.cmake
# Brief: 解析 ncnn 包配置并把 NCNN 相关 include/link 信息注入 NanoAI_NCNN 的公共依赖列表。
#
# Notes:
# - 脚本优先走标准 `find_package(ncnn CONFIG)`，兼容历史变量只是为了平滑旧工程迁移。
# - 该文件假定 NCNN 由外部预先构建并安装，不负责源码级下载或编译。

include_guard(GLOBAL)

set(NANOAI_NCNN_NCNN_CONFIG_DIR "" CACHE PATH "ncnnConfig.cmake directory (optional, same as ncnn_DIR)")

if(NANOAI_NCNN_WITH_NCNN)
    set(_NANOAI_NCNN_NCNN_FOUND FALSE)

    # 历史工程可能仍传 `NANOAI_NCNN_NCNN_CONFIG_DIR`，这里统一映射到标准 `ncnn_DIR`。
    if(NOT NANOAI_NCNN_NCNN_CONFIG_DIR STREQUAL "")
        set(ncnn_DIR "${NANOAI_NCNN_NCNN_CONFIG_DIR}" CACHE PATH "ncnn package config directory" FORCE)
    endif()

    # 无论是否显式传兼容变量，都统一走 CONFIG 模式，确保使用 ncnn 官方/外部安装导出的目标。
    find_package(ncnn CONFIG QUIET)

    if(TARGET ncnn)
        list(APPEND NANOAI_NCNN_LINK_LIBS ncnn)

        # 某些 ncnn 包把 include 导出成 `<prefix>/include/ncnn`，
        # 但本项目头文件采用 `<ncnn/net.h>` 形式引用，因此自动回退到父目录。
        get_target_property(_ncnn_inc_dirs ncnn INTERFACE_INCLUDE_DIRECTORIES)
        if(_ncnn_inc_dirs)
            foreach(_ncnn_inc_dir IN LISTS _ncnn_inc_dirs)
                if(_ncnn_inc_dir MATCHES "/ncnn$")
                    get_filename_component(_ncnn_inc_parent "${_ncnn_inc_dir}" DIRECTORY)
                    list(APPEND NANOAI_NCNN_INCLUDE_DIRS ${_ncnn_inc_parent})
                endif()
            endforeach()
        endif()

        set(_NANOAI_NCNN_NCNN_FOUND TRUE)
    endif()

    if(NOT _NANOAI_NCNN_NCNN_FOUND)
        # 缺失 ncnn 目标时直接 fail fast，避免后续头文件错误在更深层目标中才暴露。
        message(FATAL_ERROR
            "NCNN dependency is enabled but no ncnn package target was found. "
            "Please provide -Dncnn_DIR (or -DNANOAI_NCNN_NCNN_CONFIG_DIR for compatibility)."
        )
    endif()
endif()
