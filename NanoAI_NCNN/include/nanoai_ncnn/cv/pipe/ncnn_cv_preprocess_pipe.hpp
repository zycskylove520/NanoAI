// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: ncnn_cv_preprocess_pipe.hpp
// Brief: 提供 NCNN 视觉 pipeline 的常用预处理 pipe，包括 letterbox 与 RGB normalize。
//
// Design notes:
// - 预处理阶段统一输出可被 NCNN 直接消费的 `ncnn::Mat` 或携带几何信息的中间包。
// - 这里的输入校验偏严格，优先在靠近数据入口的位置尽早暴露错误。
//

#pragma once

#include <array>
#include <algorithm>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include <opencv2/opencv.hpp>

#include "nanoai_flow/core/pipe.hpp"
#include "nanoai_ncnn/cv/ncnn_cv_types.hpp"

namespace NanoAI_NCNN::CV
{
    using NanoAI_FLOW::nanoai_u32;
    using NanoAI_FLOW::NanoPipe;
    using NanoAI_FLOW::PipeExecutionPolicy;

    template <
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class NcnnCvPreprocessPipe_LetterBox : public NanoPipe<NcnnCvPreprocessPipe_LetterBox<NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        NcnnCvPreprocessPipe_LetterBox() = default;

        explicit NcnnCvPreprocessPipe_LetterBox(int input_width, int input_height)
            : input_width_(input_width), input_height_(input_height)
        {
        }

        explicit NcnnCvPreprocessPipe_LetterBox(int input_width, int input_height, const std::array<int, 3> &fill_value)
            : input_width_(input_width), input_height_(input_height), fill_value_(fill_value)
        {
        }

        template <typename T>
        auto on_run(const std::shared_ptr<NcnnRuntimeContext> &runtime, T &&in)
        {
            if (!runtime || !runtime->net)
            {
                throw std::invalid_argument("NcnnCvPreprocessPipe_LetterBox::on_run: runtime is null or not loaded");
            }

            if constexpr (!std::is_same_v<std::decay_t<T>, NcnnCvPipelineInput>)
            {
                throw std::invalid_argument("NcnnCvPreprocessPipe_LetterBox::on_run: unsupported input type");
            }
            else
            {
                if (in.image.empty())
                {
                    throw std::invalid_argument("NcnnCvPreprocessPipe_LetterBox::on_run: input image is empty");
                }

                cv::Mat letterboxed;
                NcnnCvPadRatio pr;

                const int original_width = in.image.cols;
                const int original_height = in.image.rows;
                pr.src_width = original_width;
                pr.src_height = original_height;

                // 使用较小缩放比例保持纵横比，剩余区域通过 pad_x/pad_y 记录并在后处理中还原。
                pr.ratio = std::min(static_cast<float>(input_width_) / static_cast<float>(original_width),
                                    static_cast<float>(input_height_) / static_cast<float>(original_height));

                const int new_unpad_width = static_cast<int>(std::round(static_cast<float>(original_width) * pr.ratio));
                const int new_unpad_height = static_cast<int>(std::round(static_cast<float>(original_height) * pr.ratio));

                pr.pad_x = (input_width_ - new_unpad_width) / 2;
                pr.pad_y = (input_height_ - new_unpad_height) / 2;

                if (original_width != new_unpad_width || original_height != new_unpad_height)
                {
                    cv::resize(in.image, letterboxed, cv::Size(new_unpad_width, new_unpad_height), 0, 0, cv::INTER_LINEAR);
                }
                else
                {
                    letterboxed = in.image.clone();
                }

                const int top = static_cast<int>(std::round(static_cast<float>(pr.pad_y) - 0.1F));
                const int bottom = static_cast<int>(std::round(static_cast<float>(pr.pad_y) + 0.1F));
                const int left = static_cast<int>(std::round(static_cast<float>(pr.pad_x) - 0.1F));
                const int right = static_cast<int>(std::round(static_cast<float>(pr.pad_x) + 0.1F));
                // round ±0.1 的策略与常见 YOLO letterbox 实现保持一致，可减少边界四舍五入差异。
                cv::copyMakeBorder(letterboxed, letterboxed, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(fill_value_[0], fill_value_[1], fill_value_[2]));

                NcnnCvPacket_LetterBox out;
                out.image = std::move(letterboxed);
                out.pr = pr;
                out.user_data = std::forward<T>(in).user_data;

                return std::make_tuple(runtime, std::move(out));
            }
        }

    private:
        int input_width_{640};
        int input_height_{640};
        // 默认填充值 114 对应常见检测模型预处理约定，可减少边缘补齐对特征分布的扰动。
        std::array<int, 3> fill_value_{114, 114, 114};
    };

    template <
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class NcnnCvPreprocessPipe_RgbNormalize : public NanoPipe<NcnnCvPreprocessPipe_RgbNormalize<NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        NcnnCvPreprocessPipe_RgbNormalize() = default;

        explicit NcnnCvPreprocessPipe_RgbNormalize(bool convert_bgr_to_rgb)
            : convert_bgr_to_rgb_(convert_bgr_to_rgb)
        {
        }

        explicit NcnnCvPreprocessPipe_RgbNormalize(bool convert_bgr_to_rgb, const std::array<float, 3> &mean, const std::array<float, 3> &std)
            : convert_bgr_to_rgb_(convert_bgr_to_rgb), mean_(mean), std_(std)
        {
        }

        template <typename T>
        auto on_run(const std::shared_ptr<NcnnRuntimeContext> &runtime, T &&in)
        {
            if (!runtime || !runtime->net)
            {
                throw std::invalid_argument("NcnnCvPreprocessPipe_RgbNormalize::on_run: runtime is null or not loaded");
            }

            if constexpr (std::is_same_v<std::decay_t<T>, NcnnCvPipelineInput>)
            {
                if (in.image.empty())
                {
                    throw std::invalid_argument("NcnnCvPreprocessPipe_RgbNormalize::on_run: input image is empty");
                }

                NcnnCvPacket_RgbNormalize out;
                out.user_data = std::forward<T>(in).user_data;

                // 先 clone 再做颜色空间转换，避免直接修改调用方持有的原图数据。
                cv::Mat processed_image = in.image.clone();
                if (convert_bgr_to_rgb_)
                {
                    cv::cvtColor(processed_image, processed_image, cv::COLOR_BGR2RGB);
                    out.image = ncnn::Mat::from_pixels(processed_image.data, ncnn::Mat::PIXEL_RGB, processed_image.cols, processed_image.rows);
                }
                else
                {
                    out.image = ncnn::Mat::from_pixels(processed_image.data, ncnn::Mat::PIXEL_BGR, processed_image.cols, processed_image.rows);
                }
                // NCNN 原生接口直接在 `ncnn::Mat` 上做归一化，减少额外 host 临时缓冲区。
                out.image.substract_mean_normalize(mean_.data(), std_.data());

                return std::make_tuple(runtime, std::move(out));
            }
            else if constexpr (std::is_same_v<std::decay_t<T>, NcnnCvPacket_LetterBox>)
            {
                if (in.image.empty())
                {
                    throw std::invalid_argument("NcnnCvPreprocessPipe_RgbNormalize::on_run: input image is empty");
                }

                NcnnCvPacket_RgbNormalize out;
                out.pr = in.pr;
                out.user_data = std::forward<T>(in).user_data;

                // LetterBox 分支沿用相同像素转换路径，确保不同预处理组合的数值行为一致。
                cv::Mat processed_image = in.image.clone();
                if (convert_bgr_to_rgb_)
                {
                    cv::cvtColor(processed_image, processed_image, cv::COLOR_BGR2RGB);
                    out.image = ncnn::Mat::from_pixels(processed_image.data, ncnn::Mat::PIXEL_RGB, processed_image.cols, processed_image.rows);
                }
                else
                {
                    out.image = ncnn::Mat::from_pixels(processed_image.data, ncnn::Mat::PIXEL_BGR, processed_image.cols, processed_image.rows);
                }
                out.image.substract_mean_normalize(mean_.data(), std_.data());

                return std::make_tuple(runtime, std::move(out));
            }
            else
            {
                throw std::invalid_argument("NcnnCvPreprocessPipe_RgbNormalize::on_run: unsupported input type");
            }
        }

    private:
        bool convert_bgr_to_rgb_{true};
        std::array<float, 3> mean_{0.0F, 0.0F, 0.0F};
        std::array<float, 3> std_{1.0F, 1.0F, 1.0F};
    };

} // namespace NanoAI_NCNN::CV
