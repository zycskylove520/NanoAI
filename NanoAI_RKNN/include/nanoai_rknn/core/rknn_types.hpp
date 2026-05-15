// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: rknn_types.hpp
// Brief: 定义 RKNN 模块共享的模型规格、运行时上下文与张量精度辅助工具。
//
// Design notes:
// - 该文件承载跨加载、预处理、推理阶段共享的轻量值类型，避免模块间重复定义协议结构。
// - 所有类型本身不做并发保护，默认作为 pipeline 内部消息或只读配置使用。
//

#pragma once

#include <cstdint>
#include <any>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "nanoai_flow/core/types.h"
#include "nanoai_rknn/core/rknn_helper.hpp"

namespace NanoAI_RKNN
{
    // RknnModelDomain 描述模型所属任务域，便于后续按视觉/语言等方向扩展默认配置。
    enum class RknnModelDomain
    {
        cv,
        llm,
        nlp,
        unknown
    };

    // RknnTensorPrecision 描述 host 侧输入输出张量期望精度。
    // 这里表达的是业务层协议，不直接等同于模型内部量化精度。
    enum class RknnTensorPrecision
    {
        auto_select,
        float32,
        float16,
        int8
    };

    // RknnMemoryMode 区分 host 拷贝路径与 RKNN 官方 zero-copy 路径。
    // zero_copy 依赖提前绑定好的张量内存，通常用于降低大图像输入时的搬运成本。
    enum class RknnMemoryMode
    {
        host,
        zero_copy
    };

    // RknnModelSpec 描述一次模型加载请求的关键参数。
    // model_path 必填；其余字段用于选择默认前后处理与张量精度策略。
    struct RknnModelSpec
    {
        std::string model_path{};
        RknnModelDomain domain{RknnModelDomain::cv};
        RknnTensorPrecision input_precision{RknnTensorPrecision::auto_select};
        RknnTensorPrecision output_precision{RknnTensorPrecision::float32};
        RknnMemoryMode memory_mode{RknnMemoryMode::host};
    };

    // RknnRuntimeContext 持有共享 RKNN 上下文与其对应的加载规格。
    // 生命周期：ai_ctx 通过 shared_ptr 共享给多个 stage，释放时机由最后一个持有者决定。
    struct RknnRuntimeContext
    {
        std::shared_ptr<Helper::RKNNAIContext> ai_ctx{};
        RknnModelSpec spec{};
    };

    // RknnModelLoadRequest 可作为更显式的加载消息结构，为后续异步加载场景预留。
    struct RknnModelLoadRequest
    {
        std::string model_path{};
        std::shared_ptr<Helper::RKNNAIContext> ai_ctx{};
        RknnModelDomain domain{RknnModelDomain::cv};
        RknnMemoryMode memory_mode{RknnMemoryMode::host};
    };

    // 统一从普通对象或 reference_wrapper 中取得只读引用，便于 pipeline 泛型代码透传输入。
    template <typename T>
    inline const T &rknn_get_ref(const T &input, const char *)
    {
        return input;
    }

    template <typename T>
    inline const T &rknn_get_ref(const std::reference_wrapper<T> &input, const char *)
    {
        return input.get();
    }

    template <typename T>
    inline const T &rknn_get_ref(const std::reference_wrapper<const T> &input, const char *)
    {
        return input.get();
    }

    inline rknn_tensor_type rknn_to_tensor_type(RknnTensorPrecision precision)
    {
        // auto_select 当前默认映射到 float16，优先兼顾 RKNN 常见视觉模型的吞吐与兼容性。
        switch (precision)
        {
        case RknnTensorPrecision::float32:
            return RKNN_TENSOR_FLOAT32;
        case RknnTensorPrecision::float16:
            return RKNN_TENSOR_FLOAT16;
        case RknnTensorPrecision::int8:
            return RKNN_TENSOR_INT8;
        case RknnTensorPrecision::auto_select:
        default:
            return RKNN_TENSOR_FLOAT16;
        }
    }

} // namespace NanoAI_RKNN
