// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: ncnn_cv_infer_pipe.hpp
// Brief: TODO - add file summary.
//

#pragma once

#include <stdexcept>
#include <string>
#include <tuple>

#include "nanoai_flow/core/pipe.hpp"
#include "nanoai_ncnn/cv/ncnn_cv_types.hpp"

namespace NanoAI_NCNN::CV
{
    using NanoAI_FLOW::NanoPipe;
    using NanoAI_FLOW::nanoai_u32;
    using NanoAI_FLOW::PipeExecutionPolicy;

    template <
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class NcnnCvInferPipe : public NanoPipe<NcnnCvInferPipe<NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        explicit NcnnCvInferPipe(std::string input_name = "images", std::string output_name = "output0")
            : input_name_(std::move(input_name)),
              output_name_(std::move(output_name))
        {
        }

        auto on_run(const std::shared_ptr<NcnnRuntimeContext> &runtime, const NcnnCvPacket_RgbNormalize &input)
        {
            return on_run_impl(runtime, input);
        }

        auto on_run(const std::shared_ptr<NcnnRuntimeContext> &runtime, NcnnCvPacket_RgbNormalize &&input)
        {
            return on_run_impl(runtime, std::move(input));
        }

    private:
        template <typename T>
        auto on_run_impl(const std::shared_ptr<NcnnRuntimeContext> &runtime, T &&input)
        {
            if (!runtime || !runtime->net)
            {
                throw std::invalid_argument("NcnnCvInferPipe::on_run: runtime is null or not loaded");
            }

            ncnn::Extractor extractor = runtime->net->create_extractor();
            const int input_ret = extractor.input(input_name_.c_str(), input.image);
            if (input_ret != 0)
            {
                throw std::runtime_error("NcnnCvInferPipe::on_run: extractor.input failed");
            }

            NcnnCvInferResult result;
            result.pr = input.pr;
            result.user_data = std::forward<T>(input).user_data;

            const int extract_ret = extractor.extract(output_name_.c_str(), result.output);
            if (extract_ret != 0)
            {
                throw std::runtime_error("NcnnCvInferPipe::on_run: extractor.extract failed");
            }

            return std::make_tuple(runtime, std::move(result));
        }

        std::string input_name_;
        std::string output_name_;
    };

} // namespace NanoAI_NCNN::CV
