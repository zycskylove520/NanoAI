#pragma once

#include <string>
#include <tuple>
#include <utility>

#include "nanoai_flow/core/pipe.hpp"
#include "nanoai_ncnn/core/ncnn_runtime.hpp"

namespace NanoAI_NCNN::CV
{
    using NanoAI_FLOW::NanoPipe;
    using NanoAI_FLOW::PipeCount;
    using NanoAI_FLOW::PipeExecutionPolicy;

    template <
        PipeCount NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        PipeCount DedicatedPoolSize = 0>
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
