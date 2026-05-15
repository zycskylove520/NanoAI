// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: rknn_cv_infer_pipe.hpp
// Brief: RKNN CV 推理阶段，执行模型推理并输出结果张量。
//
// Design notes:
// - 参照 NcnnCvInferPipe 模式，on_run 使用显式参数类型。
// - 接收 host/zero_copy 预处理包，输出 (ai_ctx, RknnCvInferResult)。

#pragma once

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "nanoai_flow/core/pipe.hpp"
#include "nanoai_rknn/cv/rknn_cv_types.hpp"

namespace NanoAI_RKNN::CV
{
    using NanoAI_FLOW::NanoPipe;
    using NanoAI_FLOW::nanoai_u32;
    using NanoAI_FLOW::PipeExecutionPolicy;

    namespace details
    {
        template <typename Packet>
        struct is_rknn_packet_variant : std::false_type
        {
        };

        template <typename... Ts>
        struct is_rknn_packet_variant<std::variant<Ts...>> : std::bool_constant<(... && (std::is_same_v<Ts, RknnCvPacket_HostTensor> || std::is_same_v<Ts, RknnCvPacket_ZeroCopyTensor>))>
        {
        };

        inline std::vector<RknnHostTensor> rknn_pick_input_tensors(const RknnHostInputPacket &packet)
        {
            if (packet.tensors.empty())
            {
                return {};
            }

            // 这里返回值拷贝是可接受的：输入张量通常数量很少，换来更简单稳定的调用语义。
            return packet.tensors;
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
        /// Host 模式推理（左值引用）。
        auto on_run(const std::shared_ptr<Helper::RKNNAIContext> &ai_ctx, const RknnCvPacket_HostTensor &host_packet)
        {
            return on_run_impl(ai_ctx, host_packet);
        }

        /// Host 模式推理（右值引用）。
        auto on_run(const std::shared_ptr<Helper::RKNNAIContext> &ai_ctx, RknnCvPacket_HostTensor &&host_packet)
        {
            return on_run_impl(ai_ctx, std::move(host_packet));
        }

        /// Zero-copy 模式推理（左值引用）。
        auto on_run(const std::shared_ptr<Helper::RKNNAIContext> &ai_ctx, const RknnCvPacket_ZeroCopyTensor &zero_copy_packet)
        {
            return on_run_impl(ai_ctx, zero_copy_packet);
        }

        /// Zero-copy 模式推理（右值引用）。
        auto on_run(const std::shared_ptr<Helper::RKNNAIContext> &ai_ctx, RknnCvPacket_ZeroCopyTensor &&zero_copy_packet)
        {
            return on_run_impl(ai_ctx, std::move(zero_copy_packet));
        }

        template <typename PacketVariant,
                  typename = std::enable_if_t<details::is_rknn_packet_variant<std::decay_t<PacketVariant>>::value>>
        auto on_run(const std::shared_ptr<Helper::RKNNAIContext> &ai_ctx, PacketVariant &&packet_variant)
        {
            return std::visit(
                [&](auto &&packet) {
                    return on_run_impl(ai_ctx, std::forward<decltype(packet)>(packet));
                },
                std::forward<PacketVariant>(packet_variant));
        }

    private:
        template <typename T>
        auto on_run_impl(const std::shared_ptr<Helper::RKNNAIContext> &ai_ctx, T &&input)
        {
            details::rknn_validate_context(ai_ctx, "RknnInferPipe::on_run");
            auto &ctx = *ai_ctx;

            RknnCvInferResult cv_result;

            if constexpr (Mode == RknnMemoryMode::host)
            {
                using InputType = std::decay_t<T>;
                static_assert(std::is_same_v<InputType, RknnCvPacket_HostTensor>,
                              "Host infer pipe only accepts RknnCvPacket_HostTensor");

                const auto &host_packet_wrapper = input;
                cv_result.pr = host_packet_wrapper.pr;
                cv_result.user_data = host_packet_wrapper.user_data;

                const auto input_tensors = details::rknn_pick_input_tensors(host_packet_wrapper.packet);
                if (input_tensors.empty())
                {
                    throw std::invalid_argument("RknnInferPipe::on_run: host input tensors are empty");
                }

                // host 模式按调用时提供的张量列表逐个组装 rknn_input，支持多输入模型扩展。
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
                using InputType = std::decay_t<T>;
                static_assert(std::is_same_v<InputType, RknnCvPacket_ZeroCopyTensor>,
                              "Zero-copy infer pipe only accepts RknnCvPacket_ZeroCopyTensor");

                const auto &zero_copy_packet = input;
                cv_result.pr = zero_copy_packet.pr;
                cv_result.user_data = zero_copy_packet.user_data;
            }

            int ret = rknn_run(ctx.ctx_, nullptr);
            if (ret < 0)
            {
                throw std::runtime_error("RknnInferPipe::on_run: rknn_run failed, ret=" + std::to_string(ret));
            }

            // 当前统一请求 want_float=1，优先保证后处理代码不必感知底层量化细节。
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
                // shape 直接沿用 RKNN 输出维度定义，交由具体后处理逻辑解释轴语义。
                tensor.shape.assign(attr.dims, attr.dims + attr.n_dims);
                tensor.data.resize(static_cast<NanoAI_FLOW::nanoai_usize>(attr.n_elems));
                const auto *src = static_cast<const float *>(outputs[static_cast<NanoAI_FLOW::nanoai_usize>(i)].buf);
                std::copy(src, src + attr.n_elems, tensor.data.begin());
                cv_result.outputs.emplace_back(std::move(tensor));
            }

            rknn_outputs_release(ctx.ctx_, out_count, outputs.data());
            return std::make_tuple(ai_ctx, std::move(cv_result));
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
