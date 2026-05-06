#pragma once

#include "../core/pipe.hpp"

#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

namespace NanoAI_FLOW
{

template <
    typename Derived,
    PipeCount NumThreads = 0,
    PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
    PipeCount DedicatedPoolSize = 0>
class LoadModelPipe : public NanoPipe<Derived, NumThreads, Policy, DedicatedPoolSize>
{
public:
    bool is_model_loaded() const noexcept
    {
        std::lock_guard lock(load_state_mtx_);
        return loaded_;
    }

    void reset_model_loaded() noexcept
    {
        std::lock_guard lock(load_state_mtx_);
        loaded_ = false;
    }

    template <typename T>
    decltype(auto) on_run(T &&input)
    {
        ensure_model_loaded(input);
        return std::forward<T>(input);
    }

    template <typename T>
    decltype(auto) on_run(T &&input) const
    {
        ensure_model_loaded(input);
        return std::forward<T>(input);
    }

private:
    template <typename T>
    void ensure_model_loaded(const T &input) const
    {
        std::lock_guard lock(load_state_mtx_);
        if (loaded_)
        {
            return;
        }

        const int ret = static_cast<const Derived *>(this)->load_model(input);
        if (ret != 0)
        {
            throw std::runtime_error("LoadModelPipe::load_model failed, ret=" + std::to_string(ret));
        }
        loaded_ = true;
    }

    mutable std::mutex load_state_mtx_;
    mutable bool loaded_{false};
};

template <
    typename Derived,
    PipeCount NumThreads = 0,
    PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
    PipeCount DedicatedPoolSize = 0>
class InferModelPipe : public NanoPipe<Derived, NumThreads, Policy, DedicatedPoolSize>
{
public:
    template <typename T>
    decltype(auto) on_run(T &&input)
    {
        return static_cast<Derived *>(this)->infer(std::forward<T>(input));
    }

    template <typename T>
    decltype(auto) on_run(T &&input) const
    {
        return static_cast<const Derived *>(this)->infer(std::forward<T>(input));
    }
};

} // namespace NanoAI_FLOW
