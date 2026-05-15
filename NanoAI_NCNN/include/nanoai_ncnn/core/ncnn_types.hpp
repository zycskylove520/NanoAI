// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: ncnn_types.hpp
// Brief: 定义 NCNN 模块共享的模型规格与运行时上下文类型。
//
// Design notes:
// - 该文件承载跨加载、预处理、推理阶段流转的基础协议结构。
// - 所有类型默认作为轻量配置或共享句柄容器使用，不承担并发保护职责。
//

#pragma once

#include <any>
#include <memory>
#include <string>
#include <vector>

#include <ncnn/net.h>
#include <ncnn/mat.h>

#include "nanoai_flow/core/types.h"

namespace NanoAI_NCNN
{
    // NcnnModelDomain 描述模型所属任务域，当前主要服务 CV，后续可扩展更多领域特化逻辑。
    enum class NcnnModelDomain : unsigned char
    {
        cv = 0,
        custom = 255,
    };

    // NcnnModelSpec 描述装载 NCNN 模型所需的关键输入与运行选项。
    // param/bin 路径需要成对提供；线程数与 Vulkan 开关影响后续 extractor 的执行行为。
    struct NcnnModelSpec
    {
        std::string param_path;
        std::string bin_path;
        NcnnModelDomain domain{NcnnModelDomain::cv};
        int num_threads{2};
        bool use_vulkan{false};
    };

    // NcnnRuntimeContext 聚合共享 `ncnn::Net`、对应模型规格和逻辑加载标记。
    // 生命周期：`net` 通过 shared_ptr 共享给多个 stage，最后一个持有者释放时回收资源。
    struct NcnnRuntimeContext
    {
        std::shared_ptr<ncnn::Net> net{};
        NcnnModelSpec spec{};
        // loaded 只表示当前 spec 已成功加载，不代表运行时对象不可再被重置或替换。
        bool loaded{false};
    };

} // namespace NanoAI_NCNN
