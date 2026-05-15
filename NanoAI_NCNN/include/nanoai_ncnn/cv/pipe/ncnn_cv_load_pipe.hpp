// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: ncnn_cv_load_pipe.hpp
// Brief: 定义 NCNN 视觉 pipeline 的加载首段，在首次执行时完成模型装载并透传共享 runtime。
//
// Design notes:
// - 该 pipe 兼容任意后续参数透传形式，适合作为不同视觉任务 pipeline 的统一首段。
// - 加载成功后把 runtime 作为 tuple 第一个元素输出，供下游 stage 显式依赖。
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
    // NcnnLoadPipe 负责把 `param/bin` 模型文件映射成可复用的 `ncnn::Net` 运行时。
    // 线程安全：首次加载与重载依赖 NcnnRuntimeLoader，不额外提供内部同步。
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

        // 返回共享 runtime，便于 pipeline 外部读取模型状态或手动控制复用范围。
        const std::shared_ptr<NcnnRuntimeContext> &runtime() const noexcept
        {
            return loader_.runtime();
        }

        // 修改 spec 后会强制下一次 on_run 重新装载模型文件。
        void set_model_spec(NcnnModelSpec spec)
        {
            loader_.set_model_spec(std::move(spec));
        }

        // 显式失效当前加载状态，适用于外部热更新模型或释放显存/内存场景。
        void reset_loaded()
        {
            loader_.reset_loaded();
        }

    private:
        NcnnRuntimeLoader loader_;
    };

} // namespace NanoAI_NCNN::CV
