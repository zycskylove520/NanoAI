// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: ncnn_cv_types.hpp
// Brief: TODO - add file summary.
//

#pragma once

#include <any>
#include <memory>

#include <opencv2/opencv.hpp>

#include "nanoai_ncnn/core/ncnn_types.hpp"

namespace NanoAI_NCNN::CV
{
    struct NcnnImagePacket
    {
        std::shared_ptr<NcnnRuntimeContext> runtime{};
        cv::Mat image{};
        std::any user_data{};
    };

    struct NcnnPipelineInput
    {
        cv::Mat image{};
        std::any user_data{};
    };

    struct NcnnPreprocessPacket
    {
        std::shared_ptr<NcnnRuntimeContext> runtime{};
        ncnn::Mat input{};
        NcnnCvMeta meta{};
        std::any user_data{};
    };

    inline NcnnImagePacket ncnn_make_image_packet(const NcnnPipelineInput &input, const std::shared_ptr<NcnnRuntimeContext> &runtime)
    {
        NcnnImagePacket out;
        out.runtime = runtime;
        out.image = input.image;
        out.user_data = input.user_data;
        return out;
    }

    inline NcnnImagePacket ncnn_make_image_packet(const NcnnImagePacket &input, const std::shared_ptr<NcnnRuntimeContext> &runtime)
    {
        NcnnImagePacket out = input;
        if (!out.runtime)
        {
            out.runtime = runtime;
        }
        return out;
    }

    inline NcnnImagePacket ncnn_make_image_packet(const cv::Mat &input, const std::shared_ptr<NcnnRuntimeContext> &runtime)
    {
        NcnnImagePacket out;
        out.runtime = runtime;
        out.image = input;
        return out;
    }

} // namespace NanoAI_NCNN::CV
