#pragma once

#include <functional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include "nanoai_flow/core/pipe.hpp"
#include "nanoai_rknn/core/rknn_types.hpp"

namespace NanoAI_RKNN::CV
{
    using NanoAI_FLOW::NanoPipe;
    using NanoAI_FLOW::nanoai_u32;
    using NanoAI_FLOW::PipeExecutionPolicy;

    template <
        typename Out = RknnInferResult,
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class RknnPostprocessPipe : public NanoPipe<RknnPostprocessPipe<Out, NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        using Callback = std::function<Out(const RknnInferResult &)>;

        explicit RknnPostprocessPipe(Callback callback = nullptr)
            : postprocess_cb_(std::move(callback))
        {
        }

        template <typename... Args>
        Out on_run(Args &&...args)
        {
            static_assert(sizeof...(Args) >= 2, "RknnPostprocessPipe::on_run requires (ai_ctx, infer_result)");

            auto packed = std::forward_as_tuple(std::forward<Args>(args)...);
            const auto &result = rknn_get_ref<RknnInferResult>(std::get<1>(packed), "RknnPostprocessPipe::on_run");

            if (postprocess_cb_)
            {
                return postprocess_cb_(result);
            }

            if constexpr (std::is_same_v<Out, RknnInferResult>)
            {
                return result;
            }

            throw std::invalid_argument("RknnPostprocessPipe::on_run: postprocess callback is empty");
        }

    private:
        Callback postprocess_cb_{};
    };

    template <
        typename Out = RknnInferResult,
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    using RknnHostPostprocessPipe = RknnPostprocessPipe<Out, NumThreads, Policy, DedicatedPoolSize>;

    template <
        typename Out = RknnInferResult,
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    using RknnZeroCopyPostprocessPipe = RknnPostprocessPipe<Out, NumThreads, Policy, DedicatedPoolSize>;

} // namespace NanoAI_RKNN
