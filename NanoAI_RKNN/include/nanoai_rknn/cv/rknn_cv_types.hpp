// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: rknn_cv_types.hpp
// Brief: 定义 RKNN 视觉 pipeline 在预处理、推理与后处理阶段流转的数据包结构。
//
// Design notes:
// - 这些类型只描述阶段间协议，不承载复杂行为，便于组合不同 pipe 形成自定义视觉流程。
// - user_data 字段用于无侵入透传业务上下文，避免每个 stage 重新定义扩展结构。
//

#pragma once

#include <any>
#include <memory>
#include <vector>

#include <opencv2/opencv.hpp>

#include "nanoai_rknn/core/rknn_types.hpp"

namespace NanoAI_RKNN::CV
{
    /*--------------------------User Input-----------------------*/
    // RknnPipelineInput 是视觉 pipeline 的原始入口，约定 image 为已成功解码的图像。
    struct RknnPipelineInput
    {
        cv::Mat image{};
        std::any user_data{};
    };

    /*--------------------------Preprocess Operation-----------------------*/
    // RknnCvPadRatio 记录 letterbox 缩放与补边信息，供检测框坐标反变换时恢复原图尺度。
    struct RknnCvPadRatio
    {
        float ratio{1.0F};
        int pad_x{0};
        int pad_y{0};
        int src_width{0};
        int src_height{0};
    };

    // LetterBox 结果同时保留图像与 pad 参数，避免后处理阶段重新估算几何关系。
    struct RknnCvPacket_LetterBox
    {
        cv::Mat image{};
        RknnCvPadRatio pr{};
        std::any user_data{};
    };

    // RgbNormalize 结果与 LetterBox 结构保持一致，便于需要可视化中间图像时直接复用。
    struct RknnCvPacket_RgbNormalize
    {
        cv::Mat image{};
        RknnCvPadRatio pr{};
        std::any user_data{};
    };

    // RknnHostTensor 描述 host 模式下的一块输入张量字节数据。
    // bytes 由上游持有实际内存，推理阶段会在调用 RKNN API 时只读访问。
    struct RknnHostTensor
    {
        int index{0};
        RknnTensorPrecision precision{RknnTensorPrecision::float16};
        std::vector<int> shape{};
        std::vector<NanoAI_FLOW::nanoai_u8> bytes{};
    };

    // HostInputPacket 用于承载一个或多个 host 输入张量，支持未来多输入模型扩展。
    struct RknnHostInputPacket
    {
        std::vector<RknnHostTensor> tensors{};
        std::any user_data{};
    };

    // RknnInferPacket 是 zero-copy 路径下的轻量占位包，实际张量数据已写入共享 RKNN 内存。
    struct RknnInferPacket
    {
        std::vector<RknnHostTensor> tensors{};
        std::any user_data{};
    };

    // RknnOutputTensor 统一用 float 容器暴露输出，便于后处理逻辑直接做数值计算。
    struct RknnOutputTensor
    {
        int index{0};
        RknnTensorPrecision precision{RknnTensorPrecision::float32};
        std::vector<int> shape{};
        std::vector<float> data{};
    };

    // RknnInferResult 是推理阶段的标准输出，保留所有输出张量与业务透传字段。
    struct RknnInferResult
    {
        std::vector<RknnOutputTensor> outputs{};
        std::any user_data{};
    };

    // HostTensor 包把 host 输入张量与几何信息打包，供推理和检测后处理共同使用。
    struct RknnCvPacket_HostTensor
    {
        RknnHostInputPacket packet{};
        RknnCvPadRatio pr{};
        std::any user_data{};
    };

    // ZeroCopyTensor 包只透传几何信息与 user_data；像素数据已落在 ai_ctx 绑定的 IO 内存中。
    struct RknnCvPacket_ZeroCopyTensor
    {
        RknnInferPacket packet{};
        RknnCvPadRatio pr{};
        std::any user_data{};
    };

    /*--------------------------Infer Operation-----------------------*/
    // RknnCvInferResult 在通用推理结果之外补充 pad 参数，便于检测类后处理恢复到原图坐标系。
    struct RknnCvInferResult
    {
        std::vector<RknnOutputTensor> outputs{};
        RknnCvPadRatio pr{};
        std::any user_data{};
    };

} // namespace NanoAI_RKNN