// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: rknn_cv_load_pipe.hpp
// Brief: RKNN 模型加载阶段，首次调用触发加载，后续透传输入。
//
// Design notes:
// - 参照 NcnnLoadPipe 模式，兼容 NanoAIFlow 新旧调用方式。
// - 加载完成后将 ai_ctx 作为 tuple 第一个元素传递给下游 stage。

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
    // RknnLoadPipe 负责在 pipeline 首段惰性完成模型加载，并把共享 ai_ctx 透传给后续 stage。
    // 线程安全：首次加载依赖 RknnRuntimeLoader，不做额外同步，建议单实例串行预热。
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

        /// 加载模型并透传输入（左值引用版本）。
        /// 返回值把 ai_ctx 放在 tuple 首位，以适配后续显式依赖运行时上下文的 pipe。
        auto on_run(const RknnPipelineInput &input)
        {
            loader_.ensure_loaded();
            return std::make_tuple(loader_.context(), input);
        }

        /// 加载模型并透传输入（右值引用版本）。
        auto on_run(RknnPipelineInput &&input)
        {
            loader_.ensure_loaded();
            return std::make_tuple(loader_.context(), std::move(input));
        }

        /// 兼容新 Flow API：允许透传任意下游输入。
        /// 这样 load pipe 可复用于非 CV 场景，只要后续 stage 接受 (ai_ctx, ...args) 形态即可。
        template <typename... Args>
        auto on_run(Args &&...args)
        {
            loader_.ensure_loaded();
            return std::make_tuple(loader_.context(), std::forward<Args>(args)...);
        }

        // 暴露完整 runtime 视图，方便业务侧读取最终生效的 model spec。
        const RknnRuntimeContext &runtime() const noexcept
        {
            return loader_.runtime();
        }

        // 暴露共享 ai_ctx，常用于 pipeline 外部手动释放底层 RKNN 资源。
        const std::shared_ptr<Helper::RKNNAIContext> &get_context() const noexcept
        {
            return loader_.context();
        }

        // 修改模型路径后，下一次 on_run 会重新执行加载流程。
        void set_model_path(std::string model_path)
        {
            loader_.set_model_path(std::move(model_path));
        }

        // 修改完整 spec 时会保留模板参数 Mode 作为最终内存模式来源。
        void set_model_spec(RknnModelSpec spec)
        {
            loader_.set_model_spec(std::move(spec));
        }

        // 当外部手动回收或替换底层资源后，可调用此接口强制下一次重新初始化。
        void reset_loaded() noexcept
        {
            loader_.reset_loaded();
        }

    private:
        RknnRuntimeLoader<Mode> loader_;
    };
} // namespace NanoAI_RKNN
