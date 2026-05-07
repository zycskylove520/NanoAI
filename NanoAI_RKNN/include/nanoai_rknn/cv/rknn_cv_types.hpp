// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: rknn_cv_types.hpp
// Brief: TODO - add file summary.
//

#pragma once

#include <any>
#include <memory>

#include "nanoai_rknn/core/rknn_types.hpp"

namespace NanoAI_RKNN::CV
{
    struct RknnImagePacket
    {
        std::shared_ptr<Helper::RKNNAIContext> ai_ctx{};
        const unsigned char *data{nullptr};
        int width{0};
        int height{0};
        int channels{3};
        std::any user_data{};
    };

    struct RknnPipelineInput
    {
        const unsigned char *data{nullptr};
        int width{0};
        int height{0};
        int channels{3};
        std::any user_data{};
    };

    inline RknnImagePacket rknn_make_image_packet(const RknnImagePacket &input, const RknnRuntimeContext &runtime, const char *)
    {
        RknnImagePacket out = input;
        if (!out.ai_ctx)
        {
            out.ai_ctx = runtime.ai_ctx;
        }
        return out;
    }

    inline RknnImagePacket rknn_make_image_packet(const RknnPipelineInput &input, const RknnRuntimeContext &runtime, const char *)
    {
        RknnImagePacket out;
        out.ai_ctx = runtime.ai_ctx;
        out.data = input.data;
        out.width = input.width;
        out.height = input.height;
        out.channels = input.channels;
        out.user_data = input.user_data;
        return out;
    }

} // namespace NanoAI_RKNN