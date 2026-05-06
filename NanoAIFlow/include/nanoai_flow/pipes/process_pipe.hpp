#pragma once

#include "../core/pipe.hpp"

#include <utility>

namespace NanoAI_FLOW
{

template <
    typename Derived,
    PipeCount NumThreads = 0,
    PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
    PipeCount DedicatedPoolSize = 0>
class PreprocessPipe : public NanoPipe<Derived, NumThreads, Policy, DedicatedPoolSize>
{
public:
    template <typename T>
    decltype(auto) on_run(T &&input)
    {
        return static_cast<Derived *>(this)->preprocess(std::forward<T>(input));
    }

    template <typename T>
    decltype(auto) on_run(T &&input) const
    {
        return static_cast<const Derived *>(this)->preprocess(std::forward<T>(input));
    }
};

template <
    typename Derived,
    PipeCount NumThreads = 0,
    PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
    PipeCount DedicatedPoolSize = 0>
class PostProcessPipe : public NanoPipe<Derived, NumThreads, Policy, DedicatedPoolSize>
{
public:
    template <typename T>
    decltype(auto) on_run(T &&input)
    {
        return static_cast<Derived *>(this)->postprocess(std::forward<T>(input));
    }

    template <typename T>
    decltype(auto) on_run(T &&input) const
    {
        return static_cast<const Derived *>(this)->postprocess(std::forward<T>(input));
    }
};

} // namespace NanoAI_FLOW
