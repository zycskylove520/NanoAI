// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: rknn_cv_load_pipe.hpp
// Brief: TODO - add file summary.
//

#pragma once

#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include "nanoai_flow/core/pipe.hpp"
#include "nanoai_rknn/core/rknn_runtime.hpp"

namespace NanoAI_RKNN::CV
{
    using NanoAI_FLOW::NanoPipe;
    using NanoAI_FLOW::nanoai_u32;
    using NanoAI_FLOW::PipeExecutionPolicy;

    template <
        RknnMemoryMode Mode,
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class RknnLoadPipe : public NanoPipe<RknnLoadPipe<Mode, NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        explicit RknnLoadPipe(
            RknnModelSpec spec,
            std::shared_ptr<Helper::RKNNAIContext> ai_ctx = nullptr,
            RknnAfterLoadCallback after_load = nullptr) noexcept
            : loader_(std::move(spec), std::move(ai_ctx), std::move(after_load))
        {
        }

        explicit RknnLoadPipe(
            std::string model_path,
            std::shared_ptr<Helper::RKNNAIContext> ai_ctx = nullptr,
            RknnAfterLoadCallback after_load = nullptr) noexcept
            : loader_(RknnModelSpec{std::move(model_path), RknnModelDomain::cv, RknnTensorPrecision::auto_select, RknnTensorPrecision::float32, Mode},
                      std::move(ai_ctx),
                      std::move(after_load))
        {
        }

        template <typename... Args>
        auto on_run(Args &&...args)
        {
            loader_.ensure_loaded();
            return std::make_tuple(loader_.runtime().ai_ctx, std::forward<Args>(args)...);
        }

        const RknnRuntimeContext &runtime() const noexcept
        {
            return loader_.runtime();
        }

        const std::shared_ptr<Helper::RKNNAIContext> &get_context() const noexcept
        {
            return loader_.context();
        }

        void set_model_path(std::string model_path)
        {
            loader_.set_model_path(std::move(model_path));
        }

        void set_model_spec(RknnModelSpec spec)
        {
            loader_.set_model_spec(std::move(spec));
        }

        void reset_loaded() noexcept
        {
            loader_.reset_loaded();
        }

    private:
        RknnRuntimeLoader<Mode> loader_;
    };

    template <
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    using RknnHostLoadPipe = RknnLoadPipe<RknnMemoryMode::host, NumThreads, Policy, DedicatedPoolSize>;

    template <
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    using RknnZeroCopyLoadPipe = RknnLoadPipe<RknnMemoryMode::zero_copy, NumThreads, Policy, DedicatedPoolSize>;

    using RknnAfterZeroCopyLoadCallback = RknnAfterLoadCallback;

} // namespace NanoAI_RKNN
