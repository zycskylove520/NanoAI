// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: ncnn_cv_load_pipe.hpp
// Brief: TODO - add file summary.
//

#pragma once

#include <string>
#include <tuple>
#include <utility>

#include "nanoai_flow/core/pipe.hpp"
#include "nanoai_ncnn/core/ncnn_runtime.hpp"

namespace NanoAI_NCNN::CV
{
    using NanoAI_FLOW::NanoPipe;
    using NanoAI_FLOW::nanoai_u32;
    using NanoAI_FLOW::PipeExecutionPolicy;

    template <
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class NcnnLoadPipe : public NanoPipe<NcnnLoadPipe<NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        explicit NcnnLoadPipe(NcnnModelSpec spec, std::shared_ptr<NcnnRuntimeContext> runtime = nullptr)
            : loader_(std::move(spec), std::move(runtime))
        {
        }

        explicit NcnnLoadPipe(std::string param_path, std::string bin_path, int num_threads = 2, bool use_vulkan = false)
            : loader_(NcnnModelSpec{std::move(param_path), std::move(bin_path), NcnnModelDomain::cv, num_threads, use_vulkan})
        {
        }

        template <typename... Args>
        auto on_run(Args &&...args)
        {
            loader_.ensure_loaded();
            return std::make_tuple(loader_.runtime(), std::forward<Args>(args)...);
        }

        const std::shared_ptr<NcnnRuntimeContext> &runtime() const noexcept
        {
            return loader_.runtime();
        }

        void set_model_spec(NcnnModelSpec spec)
        {
            loader_.set_model_spec(std::move(spec));
        }

        void reset_loaded()
        {
            loader_.reset_loaded();
        }

    private:
        NcnnRuntimeLoader loader_;
    };

} // namespace NanoAI_NCNN::CV
