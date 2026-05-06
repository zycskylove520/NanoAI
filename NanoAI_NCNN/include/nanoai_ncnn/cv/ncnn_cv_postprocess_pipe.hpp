#pragma once

#include <functional>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "nanoai_flow/core/pipe.hpp"
#include "nanoai_ncnn/core/ncnn_types.hpp"

namespace NanoAI_NCNN::CV
{
    using NanoAI_FLOW::NanoPipe;
    using NanoAI_FLOW::PipeCount;
    using NanoAI_FLOW::PipeExecutionPolicy;

    template <
        typename Out = NcnnInferResult,
        PipeCount NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        PipeCount DedicatedPoolSize = 0>
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
