// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: rknn_types.hpp
// Brief: TODO - add file summary.
//

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "nanoai_flow/core/types.h"
#include "nanoai_rknn/core/rknn_helper.hpp"

namespace NanoAI_RKNN
{
    enum class RknnModelDomain
    {
        cv,
        llm,
        nlp,
        unknown
    };

    enum class RknnTensorPrecision
    {
        auto_select,
        float32,
        float16,
        int8
    };

    enum class RknnMemoryMode
    {
        host,
        zero_copy
    };

    struct RknnModelSpec
    {
        std::string model_path{};
        RknnModelDomain domain{RknnModelDomain::cv};
        RknnTensorPrecision input_precision{RknnTensorPrecision::auto_select};
        RknnTensorPrecision output_precision{RknnTensorPrecision::float32};
        RknnMemoryMode memory_mode{RknnMemoryMode::host};
    };

    struct RknnRuntimeContext
    {
        std::shared_ptr<Helper::RKNNAIContext> ai_ctx{};
        RknnModelSpec spec{};
    };

    struct RknnModelLoadRequest
    {
        std::string model_path{};
        std::shared_ptr<Helper::RKNNAIContext> ai_ctx{};
        RknnModelDomain domain{RknnModelDomain::cv};
        RknnMemoryMode memory_mode{RknnMemoryMode::host};
    };

    struct RknnHostTensor
    {
        int index{0};
        RknnTensorPrecision precision{RknnTensorPrecision::float16};
        std::vector<int> shape{};
        std::vector<NanoAI_FLOW::nanoai_u8> bytes{};
    };

    struct RknnHostInputPacket
    {
        std::shared_ptr<Helper::RKNNAIContext> ai_ctx{};
        std::vector<RknnHostTensor> tensors{};

        // 兼容旧调用路径: 单输入 FP16。
        std::vector<NanoAI_FLOW::nanoai_u16> fp16_input{};
        int input_index{0};
        std::any user_data{};
    };

    struct RknnInferPacket
    {
        std::shared_ptr<Helper::RKNNAIContext> ai_ctx{};
        std::vector<RknnHostTensor> tensors{};
        std::any user_data{};
    };

    struct RknnOutputTensor
    {
        int index{0};
        RknnTensorPrecision precision{RknnTensorPrecision::float32};
        std::vector<int> shape{};
        std::vector<float> data{};
    };

    struct RknnInferResult
    {
        std::shared_ptr<Helper::RKNNAIContext> ai_ctx{};
        std::vector<RknnOutputTensor> outputs{};
        std::any user_data{};
    };

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

#include "nanoai_rknn/cv/rknn_cv_types.hpp"
