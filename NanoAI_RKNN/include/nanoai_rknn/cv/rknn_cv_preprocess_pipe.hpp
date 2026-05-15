// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: rknn_cv_preprocess_pipe.hpp
// Brief: RKNN CV 预处理阶段，将原始图像像素转换为模型输入张量。
//
// Design notes:
// - 参照 NcnnCvPreprocessPipe 模式，on_run 使用显式参数类型。
// - LetterBox 接收 (ai_ctx, RknnPipelineInput)，输出 (ai_ctx, RknnCvPacket_LetterBox)。
// - RgbNormalize 接收 (ai_ctx, RknnPipelineInput) 或 (ai_ctx, RknnCvPacket_LetterBox)，
//   根据 memory_mode 输出 host 或 zero_copy 可推理输入包。

#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <Float16.h>
#include <opencv2/opencv.hpp>
#include <rknn_api.h>

#include "nanoai_flow/core/pipe.hpp"
#include "nanoai_rknn/core/rknn_types.hpp"
#include "nanoai_rknn/cv/rknn_cv_types.hpp"

namespace NanoAI_RKNN::CV
{
    using NanoAI_FLOW::nanoai_u32;
    using NanoAI_FLOW::NanoPipe;
    using NanoAI_FLOW::PipeExecutionPolicy;

    template <
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class RknnCvPreprocessPipe_LetterBox : public NanoPipe<RknnCvPreprocessPipe_LetterBox<NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        RknnCvPreprocessPipe_LetterBox() = default;

        explicit RknnCvPreprocessPipe_LetterBox(int input_width, int input_height)
            : input_width_(input_width), input_height_(input_height)
        {
        }

        explicit RknnCvPreprocessPipe_LetterBox(int input_width, int input_height, const std::array<int, 3> &fill_value)
            : input_width_(input_width), input_height_(input_height), fill_value_(fill_value)
        {
        }

        template <typename T>
        auto on_run(const std::shared_ptr<Helper::RKNNAIContext> &ai_ctx, T &&in)
        {
            if (!ai_ctx)
            {
                throw std::invalid_argument("RknnCvPreprocessPipe_LetterBox::on_run: runtime context is null");
            }

            if constexpr (!std::is_same_v<std::decay_t<T>, RknnPipelineInput>)
            {
                throw std::invalid_argument("RknnCvPreprocessPipe_LetterBox::on_run: unsupported input type");
            }
            else
            {
                if (in.image.empty())
                {
                    throw std::invalid_argument("RknnCvPreprocessPipe_LetterBox::on_run: input image is empty");
                }

                cv::Mat letterboxed;
                RknnCvPadRatio pr;
                pr.src_width = in.image.cols;
                pr.src_height = in.image.rows;

                // 按较小缩放比例保持纵横比，后续通过 pad_x/pad_y 记录剩余边界补齐量。
                pr.ratio = std::min(static_cast<float>(input_width_) / static_cast<float>(in.image.cols),
                                    static_cast<float>(input_height_) / static_cast<float>(in.image.rows));

                const int new_unpad_width = static_cast<int>(std::round(static_cast<float>(in.image.cols) * pr.ratio));
                const int new_unpad_height = static_cast<int>(std::round(static_cast<float>(in.image.rows) * pr.ratio));

                pr.pad_x = (input_width_ - new_unpad_width) / 2;
                pr.pad_y = (input_height_ - new_unpad_height) / 2;

                if (in.image.cols != new_unpad_width || in.image.rows != new_unpad_height)
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
                // top/bottom/left/right 使用轻微偏移的 round 策略，尽量与常见 YOLO letterbox 实现保持一致。
                cv::copyMakeBorder(letterboxed, letterboxed, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(fill_value_[0], fill_value_[1], fill_value_[2]));

                RknnCvPacket_LetterBox out;
                out.image = std::move(letterboxed);
                out.pr = pr;
                out.user_data = std::forward<T>(in).user_data;
                return std::make_tuple(ai_ctx, std::move(out));
            }
        }

    private:
        int input_width_{640};
        int input_height_{640};
        std::array<int, 3> fill_value_{114, 114, 114};
    };

    template <
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class RknnCvPreprocessPipe_RgbNormalize : public NanoPipe<RknnCvPreprocessPipe_RgbNormalize<NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        RknnCvPreprocessPipe_RgbNormalize() = default;

        explicit RknnCvPreprocessPipe_RgbNormalize(RknnMemoryMode memory_mode)
            : memory_mode_(memory_mode)
        {
        }

        explicit RknnCvPreprocessPipe_RgbNormalize(bool convert_bgr_to_rgb)
            : convert_bgr_to_rgb_(convert_bgr_to_rgb)
        {
        }

        explicit RknnCvPreprocessPipe_RgbNormalize(RknnMemoryMode memory_mode, bool convert_bgr_to_rgb)
            : memory_mode_(memory_mode), convert_bgr_to_rgb_(convert_bgr_to_rgb)
        {
        }

        explicit RknnCvPreprocessPipe_RgbNormalize(bool convert_bgr_to_rgb, const std::array<float, 3> &mean, const std::array<float, 3> &std)
            : convert_bgr_to_rgb_(convert_bgr_to_rgb), mean_(mean), std_(std)
        {
        }

        explicit RknnCvPreprocessPipe_RgbNormalize(RknnMemoryMode memory_mode, bool convert_bgr_to_rgb, const std::array<float, 3> &mean, const std::array<float, 3> &std)
            : memory_mode_(memory_mode), convert_bgr_to_rgb_(convert_bgr_to_rgb), mean_(mean), std_(std)
        {
        }

        template <typename T>
        auto on_run(const std::shared_ptr<Helper::RKNNAIContext> &ai_ctx, T &&in)
        {
            if (!ai_ctx)
            {
                throw std::invalid_argument("RknnCvPreprocessPipe_RgbNormalize::on_run: runtime context is null");
            }

            if constexpr (std::is_same_v<std::decay_t<T>, RknnPipelineInput>)
            {
                if (in.image.empty())
                {
                    throw std::invalid_argument("RknnCvPreprocessPipe_RgbNormalize::on_run: input image is empty");
                }

                return build_output(ai_ctx, in.image, {}, std::forward<T>(in).user_data);
            }
            else if constexpr (std::is_same_v<std::decay_t<T>, RknnCvPacket_LetterBox>)
            {
                if (in.image.empty())
                {
                    throw std::invalid_argument("RknnCvPreprocessPipe_RgbNormalize::on_run: input image is empty");
                }

                return build_output(ai_ctx, in.image, in.pr, std::forward<T>(in).user_data);
            }
            else
            {
                throw std::invalid_argument("RknnCvPreprocessPipe_RgbNormalize::on_run: unsupported input type");
            }
        }

    private:
        RknnMemoryMode memory_mode_{RknnMemoryMode::host};
        cv::Mat make_normalized_image(const cv::Mat &input) const
        {
            if (input.empty())
            {
                throw std::invalid_argument("RknnCvPreprocessPipe_RgbNormalize::on_run: input image is empty");
            }

            int channels = input.channels();
            cv::Mat processed_image = input.clone();

            if (channels == 4)
            {
                if (convert_bgr_to_rgb_)
                {
                    cv::cvtColor(processed_image, processed_image, cv::COLOR_BGRA2RGB);
                }
                else
                {
                    cv::cvtColor(processed_image, processed_image, cv::COLOR_BGRA2BGR);
                }
                channels = 3;
            }
            else if (channels == 1)
            {
                cv::cvtColor(processed_image, processed_image, cv::COLOR_GRAY2BGR);
                if (convert_bgr_to_rgb_)
                {
                    cv::cvtColor(processed_image, processed_image, cv::COLOR_BGR2RGB);
                }
                channels = 3;
            }
            else if (convert_bgr_to_rgb_)
            {
                cv::cvtColor(processed_image, processed_image, cv::COLOR_BGR2RGB);
            }

            cv::Mat normalized;
            processed_image.convertTo(normalized, CV_32F);

            const int total = normalized.rows * normalized.cols;
            auto *ptr = reinterpret_cast<float *>(normalized.data);
            for (int i = 0; i < total; ++i)
            {
                for (int c = 0; c < channels; ++c)
                {
                    const int idx = i * channels + c;
                    ptr[idx] = (ptr[idx] - mean_[static_cast<std::size_t>(c)]) * std_[static_cast<std::size_t>(c)];
                }
            }

            return normalized;
        }

        auto build_output(const std::shared_ptr<Helper::RKNNAIContext> &ai_ctx,
                          const cv::Mat &input,
                          const RknnCvPadRatio &pr,
                          std::any user_data) const
            -> std::tuple<std::shared_ptr<Helper::RKNNAIContext>, RknnCvPacket_HostTensor>
        {
            cv::Mat normalized = make_normalized_image(input);
            if (memory_mode_ != RknnMemoryMode::host)
            {
                throw std::invalid_argument("RknnCvPreprocessPipe_RgbNormalize::build_output: host pipeline requires host memory mode");
            }

            // 当前返回类型固定为 host packet；若后续开放 zero-copy 预处理输出，可在这里做分支扩展。
            return std::make_tuple(ai_ctx, build_host_packet(ai_ctx, normalized, pr, std::move(user_data)));
        }

        RknnCvPacket_HostTensor build_host_packet(const std::shared_ptr<Helper::RKNNAIContext> &ai_ctx,
                                                  const cv::Mat &normalized,
                                                  const RknnCvPadRatio &pr,
                                                  std::any user_data) const
        {
            (void)ai_ctx;
            const int channels = normalized.channels();
            const std::size_t bytes = normalized.total() * normalized.elemSize();

            RknnHostInputPacket host_packet;

            RknnHostTensor tensor;
            tensor.index = 0;
            tensor.precision = RknnTensorPrecision::float32;
            tensor.shape = {1, normalized.rows, normalized.cols, channels};
            tensor.bytes.resize(bytes);
            // host 模式显式复制一份连续字节数据，确保下游 rknn_inputs_set 调用期间缓冲区稳定可读。
            std::memcpy(tensor.bytes.data(), normalized.data, bytes);
            host_packet.tensors.emplace_back(std::move(tensor));
            host_packet.user_data = user_data;

            RknnCvPacket_HostTensor out;
            out.packet = std::move(host_packet);
            out.pr = pr;
            out.user_data = std::move(user_data);
            out.packet.user_data = out.user_data;
            return out;
        }

        RknnCvPacket_ZeroCopyTensor build_zero_copy_packet(const std::shared_ptr<Helper::RKNNAIContext> &ai_ctx,
                                                           const cv::Mat &normalized,
                                                           const RknnCvPadRatio &pr,
                                                           std::any user_data) const
        {
            if (!ai_ctx->input_mems_ || !ai_ctx->input_mems_[0] || !ai_ctx->input_mems_[0]->virt_addr)
            {
                throw std::invalid_argument("RknnCvPreprocessPipe_RgbNormalize::on_run: zero_copy input memory is not ready");
            }

            const std::size_t bytes = normalized.total() * normalized.elemSize();
            // zero-copy 路径直接写入 RKNN 预绑定输入内存，避免额外 host 缓冲区分配。
            std::memcpy(ai_ctx->input_mems_[0]->virt_addr, normalized.data, bytes);

            RknnInferPacket infer_packet;
            infer_packet.user_data = user_data;

            RknnCvPacket_ZeroCopyTensor out;
            out.packet = std::move(infer_packet);
            out.pr = pr;
            out.user_data = std::move(user_data);
            out.packet.user_data = out.user_data;
            return out;
        }

        bool convert_bgr_to_rgb_{true};
        std::array<float, 3> mean_{0.0F, 0.0F, 0.0F};
        std::array<float, 3> std_{1.0F, 1.0F, 1.0F};
    };

} // namespace NanoAI_RKNN
