#pragma once

#include <cstring>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <Float16.h>

#include "nanoai_flow/core/pipe.hpp"
#include "nanoai_rknn/core/rknn_types.hpp"

namespace NanoAI_RKNN::CV
{
    using NanoAI_FLOW::NanoPipe;
    using NanoAI_FLOW::PipeCount;
    using NanoAI_FLOW::PipeExecutionPolicy;

    template <
        RknnTensorPrecision OutputPrecision = RknnTensorPrecision::float16,
        PipeCount NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        PipeCount DedicatedPoolSize = 0>
    class RknnCvRgbPreprocessPipe : public NanoPipe<RknnCvRgbPreprocessPipe<OutputPrecision, NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        template <typename... Args>
        auto on_run(Args &&...args)
        {
            static_assert(sizeof...(Args) >= 2, "RknnCvRgbPreprocessPipe::on_run requires (ai_ctx, image_input)");

            auto packed = std::forward_as_tuple(std::forward<Args>(args)...);
            const auto &ai_ctx = rknn_get_ref<std::shared_ptr<Helper::RKNNAIContext>>(std::get<0>(packed), "RknnCvRgbPreprocessPipe::on_run");
            RknnRuntimeContext runtime{ai_ctx};
            const auto packet = rknn_make_image_packet(std::get<1>(packed), runtime, "RknnCvRgbPreprocessPipe::on_run");

            if (!packet.ai_ctx)
            {
                throw std::invalid_argument("RknnCvRgbPreprocessPipe::on_run: runtime context is null");
            }

            if (!packet.data)
            {
                throw std::invalid_argument("RknnCvRgbPreprocessPipe::on_run: input data is null");
            }

            const int width = packet.ai_ctx->model_width_;
            const int height = packet.ai_ctx->model_height_;
            const int channels = packet.ai_ctx->model_channel_;
            const int total = width * height * channels;

            RknnHostInputPacket host_packet;
            host_packet.ai_ctx = packet.ai_ctx;
            host_packet.user_data = packet.user_data;

            RknnHostTensor tensor;
            tensor.index = 0;
            tensor.precision = OutputPrecision;
            tensor.shape = {1, height, width, channels};

            if constexpr (OutputPrecision == RknnTensorPrecision::float16)
            {
                host_packet.fp16_input.resize(static_cast<std::size_t>(total));
                for (int i = 0; i < total; ++i)
                {
                    rknpu2::float16 f16(static_cast<float>(packet.data[i]));
                    host_packet.fp16_input[static_cast<std::size_t>(i)] = f16.bits();
                }

                tensor.bytes.resize(host_packet.fp16_input.size() * sizeof(std::uint16_t));
                std::memcpy(tensor.bytes.data(), host_packet.fp16_input.data(), tensor.bytes.size());
            }
            else if constexpr (OutputPrecision == RknnTensorPrecision::float32)
            {
                std::vector<float> fp32(static_cast<std::size_t>(total));
                for (int i = 0; i < total; ++i)
                {
                    fp32[static_cast<std::size_t>(i)] = static_cast<float>(packet.data[i]);
                }
                tensor.bytes.resize(fp32.size() * sizeof(float));
                std::memcpy(tensor.bytes.data(), fp32.data(), tensor.bytes.size());
            }
            else if constexpr (OutputPrecision == RknnTensorPrecision::int8)
            {
                tensor.bytes.resize(static_cast<std::size_t>(total));
                for (int i = 0; i < total; ++i)
                {
                    tensor.bytes[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(packet.data[i]);
                }
            }
            else
            {
                throw std::invalid_argument("RknnCvRgbPreprocessPipe::on_run: unsupported precision");
            }

            host_packet.tensors.emplace_back(std::move(tensor));
            return std::make_tuple(ai_ctx, std::move(host_packet));
        }
    };

    template <
        PipeCount NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        PipeCount DedicatedPoolSize = 0>
    using RknnHostRgbToFp16Pipe = RknnCvRgbPreprocessPipe<RknnTensorPrecision::float16, NumThreads, Policy, DedicatedPoolSize>;

    template <
        PipeCount NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        PipeCount DedicatedPoolSize = 0>
    using RknnHostRgbToFp16PreprocessPipe = RknnHostRgbToFp16Pipe<NumThreads, Policy, DedicatedPoolSize>;

} // namespace NanoAI_RKNN
