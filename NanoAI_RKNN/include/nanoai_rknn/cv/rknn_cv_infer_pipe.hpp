// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: rknn_cv_infer_pipe.hpp
// Brief: TODO - add file summary.
//

#pragma once

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "nanoai_flow/core/pipe.hpp"
#include "nanoai_rknn/core/rknn_types.hpp"

namespace NanoAI_RKNN::CV
{
    using NanoAI_FLOW::NanoPipe;
    using NanoAI_FLOW::nanoai_u32;
    using NanoAI_FLOW::PipeExecutionPolicy;

    namespace details
    {
        inline std::vector<RknnHostTensor> rknn_pick_input_tensors(const RknnHostInputPacket &packet)
        {
            if (!packet.tensors.empty())
            {
                return packet.tensors;
            }

            if (packet.fp16_input.empty())
            {
                return {};
            }

            RknnHostTensor tensor;
            tensor.index = packet.input_index;
            tensor.precision = RknnTensorPrecision::float16;
            tensor.bytes.resize(packet.fp16_input.size() * sizeof(NanoAI_FLOW::nanoai_u16));
            std::memcpy(tensor.bytes.data(), packet.fp16_input.data(), tensor.bytes.size());
            return {std::move(tensor)};
        }

        inline void rknn_validate_context(const std::shared_ptr<Helper::RKNNAIContext> &ai_ctx, const char *scope)
        {
            if (!ai_ctx)
            {
                throw std::invalid_argument(std::string(scope) + ": runtime context is null");
            }
        }
    } // namespace details

    template <
        RknnMemoryMode Mode,
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class RknnInferPipe : public NanoPipe<RknnInferPipe<Mode, NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        template <typename... Args>
        auto on_run(Args &&...args)
        {
            static_assert(sizeof...(Args) >= 2, "RknnInferPipe::on_run requires (ai_ctx, infer_input)");

            auto packed = std::forward_as_tuple(std::forward<Args>(args)...);
            const auto &ai_ctx = rknn_get_ref<std::shared_ptr<Helper::RKNNAIContext>>(std::get<0>(packed), "RknnInferPipe::on_run");
            details::rknn_validate_context(ai_ctx, "RknnInferPipe::on_run");
            auto &ctx = *ai_ctx;

            RknnInferResult result;
            result.ai_ctx = ai_ctx;

            if constexpr (Mode == RknnMemoryMode::host)
            {
                const auto &host_packet = rknn_get_ref<RknnHostInputPacket>(std::get<1>(packed), "RknnInferPipe::on_run");
                result.user_data = host_packet.user_data;

                const auto input_tensors = details::rknn_pick_input_tensors(host_packet);
                if (input_tensors.empty())
                {
                    throw std::invalid_argument("RknnInferPipe::on_run: host input tensors are empty");
                }

                std::vector<rknn_input> rknn_inputs(input_tensors.size());
                std::memset(rknn_inputs.data(), 0, rknn_inputs.size() * sizeof(rknn_input));
                for (NanoAI_FLOW::nanoai_usize i = 0; i < input_tensors.size(); ++i)
                {
                    const auto &tensor = input_tensors[i];
                    if (tensor.index < 0 || tensor.index >= ctx.io_num_.n_input)
                    {
                        throw std::invalid_argument("RknnInferPipe::on_run: input tensor index out of range");
                    }

                    rknn_inputs[i].index = tensor.index;
                    rknn_inputs[i].type = rknn_to_tensor_type(tensor.precision);
                    rknn_inputs[i].fmt = ctx.input_attrs_[tensor.index].fmt;
                    rknn_inputs[i].buf = const_cast<NanoAI_FLOW::nanoai_u8 *>(tensor.bytes.data());
                    rknn_inputs[i].size = tensor.bytes.size();
                }

                int ret = rknn_inputs_set(ctx.ctx_, static_cast<NanoAI_FLOW::nanoai_u32>(rknn_inputs.size()), rknn_inputs.data());
                if (ret < 0)
                {
                    throw std::runtime_error("RknnInferPipe::on_run: rknn_inputs_set failed, ret=" + std::to_string(ret));
                }
            }
            else
            {
                const auto &infer_packet = rknn_get_ref<RknnInferPacket>(std::get<1>(packed), "RknnInferPipe::on_run");
                result.user_data = infer_packet.user_data;
            }

            int ret = rknn_run(ctx.ctx_, nullptr);
            if (ret < 0)
            {
                throw std::runtime_error("RknnInferPipe::on_run: rknn_run failed, ret=" + std::to_string(ret));
            }

            const int out_count = ctx.io_num_.n_output;
            std::vector<rknn_output> outputs(static_cast<NanoAI_FLOW::nanoai_usize>(out_count));
            std::memset(outputs.data(), 0, outputs.size() * sizeof(rknn_output));
            for (int i = 0; i < out_count; ++i)
            {
                outputs[static_cast<NanoAI_FLOW::nanoai_usize>(i)].index = i;
                outputs[static_cast<NanoAI_FLOW::nanoai_usize>(i)].want_float = 1;
            }

            ret = rknn_outputs_get(ctx.ctx_, out_count, outputs.data(), nullptr);
            if (ret < 0)
            {
                throw std::runtime_error("RknnInferPipe::on_run: rknn_outputs_get failed, ret=" + std::to_string(ret));
            }

            for (int i = 0; i < out_count; ++i)
            {
                const auto &attr = ctx.output_attrs_[i];
                RknnOutputTensor tensor;
                tensor.index = i;
                tensor.precision = RknnTensorPrecision::float32;
                tensor.shape.assign(attr.dims, attr.dims + attr.n_dims);
                tensor.data.resize(static_cast<NanoAI_FLOW::nanoai_usize>(attr.n_elems));
                const auto *src = static_cast<const float *>(outputs[static_cast<NanoAI_FLOW::nanoai_usize>(i)].buf);
                std::copy(src, src + attr.n_elems, tensor.data.begin());
                result.outputs.emplace_back(std::move(tensor));
            }

            rknn_outputs_release(ctx.ctx_, out_count, outputs.data());
            return std::make_tuple(ai_ctx, std::move(result));
        }
    };

    template <
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    using RknnHostInferPipe = RknnInferPipe<RknnMemoryMode::host, NumThreads, Policy, DedicatedPoolSize>;

    template <
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    using RknnZeroCopyInferPipe = RknnInferPipe<RknnMemoryMode::zero_copy, NumThreads, Policy, DedicatedPoolSize>;

} // namespace NanoAI_RKNN
