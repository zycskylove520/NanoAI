// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: ncnn_cv_types.hpp
// Brief: 定义 NCNN 视觉 pipeline 在输入、预处理和推理阶段共享的数据包结构。
//
// Design notes:
// - 这些类型只表达阶段协议，避免把具体模型逻辑耦合到公共数据结构中。
// - `user_data` 字段用于无侵入透传业务上下文，方便 pipeline 外部保持附加信息同步。
//

#pragma once

#include <any>
#include <memory>

#include <opencv2/opencv.hpp>

#include "nanoai_ncnn/core/ncnn_types.hpp"

namespace NanoAI_NCNN::CV
{
    /*--------------------------User Input-----------------------*/
    // NcnnCvPipelineInput 是视觉 pipeline 的原始入口，约定 image 为已经成功解码的 2D 图像。
    struct NcnnCvPipelineInput
    {
        cv::Mat image{};
        std::any user_data{};
    };

    /*--------------------------Preprocess Operation-----------------------*/
    // NcnnCvPadRatio 保存 letterbox 的缩放与补边参数，供检测框回映射到原图坐标时使用。
    struct NcnnCvPadRatio
    {
        float ratio{1.0F};
        int pad_x{0};
        int pad_y{0};
        int src_width{0};
        int src_height{0};
    };

    // LetterBox 输出包保留图像与 pad 元数据，便于后续预处理或检测后处理继续使用。
    struct NcnnCvPacket_LetterBox
    {
        cv::Mat image{};
        NcnnCvPadRatio pr{};
        std::any user_data{};
    };

    // RgbNormalize 输出包把像素转换为 `ncnn::Mat`，供推理阶段直接送入 extractor。
    struct NcnnCvPacket_RgbNormalize
    {
        ncnn::Mat image{};
        NcnnCvPadRatio pr{};
        std::any user_data{};
    };

    /*--------------------------Infer Operation-----------------------*/
    // NcnnCvInferResult 是推理阶段标准输出，包含单个输出张量及其关联几何信息。
    struct NcnnCvInferResult
    {
        ncnn::Mat output{};
        NcnnCvPadRatio pr{};
        std::any user_data{};
    };

} // namespace NanoAI_NCNN::CV
