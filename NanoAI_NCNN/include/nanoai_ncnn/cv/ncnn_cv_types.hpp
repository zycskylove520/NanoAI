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
    /*--------------------------User Input-----------------------*/
    // 2D image input from user, can be extended to support more complex input in the future
    struct NcnnCvPipelineInput
    {
        cv::Mat image{};
        std::any user_data{};
    };

    /*--------------------------Preprocess Operation-----------------------*/
    struct NcnnCvPadRatio
    {
        float ratio{1.0F};
        int pad_x{0};
        int pad_y{0};
        int src_width{0};
        int src_height{0};
    };

    struct NcnnCvPacket_LetterBox
    {
        cv::Mat image{};
        NcnnCvPadRatio pr{};
        std::any user_data{};
    };

    struct NcnnCvPacket_RgbNormalize
    {
        ncnn::Mat image{};
        NcnnCvPadRatio pr{};
        std::any user_data{};
    };

    /*--------------------------Infer Operation-----------------------*/
    struct NcnnCvInferResult
    {
        ncnn::Mat output{};
        NcnnCvPadRatio pr{};
        std::any user_data{};
    };

} // namespace NanoAI_NCNN::CV
