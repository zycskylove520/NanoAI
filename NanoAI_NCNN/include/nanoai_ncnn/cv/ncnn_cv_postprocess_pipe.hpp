// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: ncnn_cv_postprocess_pipe.hpp
// Brief: TODO - add file summary.
//

#pragma once

#include <functional>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "nanoai_flow/core/pipe.hpp"
#include "nanoai_ncnn/core/ncnn_types.hpp"

using NanoAI_FLOW::NanoPipe;
using NanoAI_FLOW::nanoai_u32;
using NanoAI_FLOW::PipeExecutionPolicy;

namespace NanoAI_NCNN::CV
{
    template <
        typename Out = NcnnInferResult,
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class NcnnCvPostprocessPipe : public NanoPipe<NcnnCvPostprocessPipe<Out, NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        using Callback = std::function<Out(const NcnnInferResult &)>;

        explicit NcnnCvPostprocessPipe(Callback callback = nullptr)
            : callback_(std::move(callback))
        {
        }

        Out on_run(const std::shared_ptr<NcnnRuntimeContext> &, const NcnnInferResult &result)
        {
            if (callback_)
            {
                return callback_(result);
            }

            if constexpr (std::is_same_v<Out, NcnnInferResult>)
            {
                return result;
            }

            throw std::invalid_argument("NcnnCvPostprocessPipe::on_run: callback is empty");
        }

    private:
        Callback callback_{};
    };

} // namespace NanoAI_NCNN::CV

namespace NanoAI_NCNN::CV
{
    template <
        typename Out = NcnnInferResult,
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class NcnnCv_Yolo26PostprocessPipe : public NcnnCvPostprocessPipe<NcnnCv_Yolo26PostprocessPipe<Out, NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    };
} // namespace NanoAI_NCNN::CV
