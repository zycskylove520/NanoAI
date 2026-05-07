#pragma once

#include <algorithm>
#include <stdexcept>
#include <tuple>

#include <opencv2/opencv.hpp>

#include "nanoai_flow/core/pipe.hpp"
#include "nanoai_ncnn/cv/ncnn_cv_types.hpp"

namespace NanoAI_NCNN::CV
{
    using NanoAI_FLOW::NanoPipe;
    using NanoAI_FLOW::nanoai_u32;
    using NanoAI_FLOW::PipeExecutionPolicy;

    inline void ncnn_letter_box(const cv::Mat &src, cv::Mat &dst, int target_width, int target_height, NcnnCvMeta &meta)
    {
        const int original_width = src.cols;
        const int original_height = src.rows;

        meta.src_width = original_width;
        meta.src_height = original_height;
        meta.dst_width = target_width;
        meta.dst_height = target_height;

        meta.ratio = std::min(static_cast<float>(target_width) / static_cast<float>(original_width),
                              static_cast<float>(target_height) / static_cast<float>(original_height));

        const int new_unpad_width = static_cast<int>(std::round(static_cast<float>(original_width) * meta.ratio));
        const int new_unpad_height = static_cast<int>(std::round(static_cast<float>(original_height) * meta.ratio));

        meta.pad_x = (target_width - new_unpad_width) / 2;
        meta.pad_y = (target_height - new_unpad_height) / 2;

        if (original_width != new_unpad_width || original_height != new_unpad_height)
        {
            cv::resize(src, dst, cv::Size(new_unpad_width, new_unpad_height), 0, 0, cv::INTER_LINEAR);
        }
        else
        {
            dst = src.clone();
        }

        const int top = static_cast<int>(std::round(static_cast<float>(meta.pad_y) - 0.1F));
        const int bottom = static_cast<int>(std::round(static_cast<float>(meta.pad_y) + 0.1F));
        const int left = static_cast<int>(std::round(static_cast<float>(meta.pad_x) - 0.1F));
        const int right = static_cast<int>(std::round(static_cast<float>(meta.pad_x) + 0.1F));
        cv::copyMakeBorder(dst, dst, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(114, 114, 114));
    }

    template <
        int InputWidth = 960,
        int InputHeight = 960,
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class NcnnCvRgbPreprocessPipe : public NanoPipe<NcnnCvRgbPreprocessPipe<InputWidth, InputHeight, NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        template <typename In>
        auto on_run(const std::shared_ptr<NcnnRuntimeContext> &runtime, In &&input)
        {
            if (!runtime || !runtime->net)
            {
                throw std::invalid_argument("NcnnCvRgbPreprocessPipe::on_run: runtime is null or not loaded");
            }

            const auto packet = ncnn_make_image_packet(std::forward<In>(input), runtime);
            if (packet.image.empty())
            {
                throw std::invalid_argument("NcnnCvRgbPreprocessPipe::on_run: input image is empty");
            }

            cv::Mat letterboxed;
            NcnnCvMeta meta;
            ncnn_letter_box(packet.image, letterboxed, InputWidth, InputHeight, meta);

            cv::cvtColor(letterboxed, letterboxed, cv::COLOR_BGR2RGB);

            NcnnPreprocessPacket out;
            out.runtime = runtime;
            out.meta = meta;
            out.user_data = packet.user_data;
            out.input = ncnn::Mat::from_pixels(letterboxed.data, ncnn::Mat::PIXEL_RGB, InputWidth, InputHeight);

            const float mean_vals[3] = {0.0F, 0.0F, 0.0F};
            const float norm_vals[3] = {0.007843F, 0.007843F, 0.007843F};
            out.input.substract_mean_normalize(mean_vals, norm_vals);

            return std::make_tuple(runtime, std::move(out));
        }
    };

} // namespace NanoAI_NCNN::CV
