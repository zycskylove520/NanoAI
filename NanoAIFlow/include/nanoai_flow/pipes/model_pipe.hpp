// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: model_pipe.hpp
// Brief: 模型生命周期与推理阶段适配器。

#pragma once

#include "../core/pipe.hpp"

#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

namespace NanoAI_FLOW
{

/**
 * @brief “首次加载，后续透传”模型加载阶段适配器。
 * @tparam Derived 派生类类型，需要实现 load_model(...)
 * @tparam NumThreads 阶段并发度配置。
 * @tparam Policy 阶段执行策略。
 * @tparam DedicatedPoolSize 专用线程池大小（仅 dedicated_pool 生效）。
 *
 * 首次请求触发 Derived::load_model(...)。
 * 成功后后续请求不再重复加载，直接透传输入。
 */
template <
    typename Derived,
    nanoai_u32 NumThreads = 0,
    PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
    nanoai_u32 DedicatedPoolSize = 0>
class LoadModelPipe : public NanoPipe<Derived, NumThreads, Policy, DedicatedPoolSize>
{
public:
    /// 查询模型是否已加载。
    bool is_model_loaded() const noexcept
    {
        std::lock_guard lock(load_state_mtx_);
        return loaded_;
    }

    /// 重置加载状态，下一次调用将重新加载模型。
    void reset_model_loaded() noexcept
    {
        std::lock_guard lock(load_state_mtx_);
        loaded_ = false;
    }

    /// 确保模型已加载，同时保持输入原样透传。
    template <typename T>
    decltype(auto) on_run(T &&input)
    {
        ensure_model_loaded(input);
        return std::forward<T>(input);
    }

    /// const 重载：同样执行“确保已加载 + 透传输入”。
    template <typename T>
    decltype(auto) on_run(T &&input) const
    {
        ensure_model_loaded(input);
        return std::forward<T>(input);
    }

private:
    /// 线程安全的一次性加载路径。
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

    /// 保护 loaded_ 与一次性加载过程。
    mutable std::mutex load_state_mtx_;
    /// load_model 成功后置为 true。
    mutable bool loaded_{false};
};

/**
 * @brief 推理阶段适配器。
 * @tparam Derived 派生类类型，需要实现 infer(...)
 * @tparam NumThreads 阶段并发度配置。
 * @tparam Policy 阶段执行策略。
 * @tparam DedicatedPoolSize 专用线程池大小（仅 dedicated_pool 生效）。
 */
template <
    typename Derived,
    nanoai_u32 NumThreads = 0,
    PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
    nanoai_u32 DedicatedPoolSize = 0>
class InferModelPipe : public NanoPipe<Derived, NumThreads, Policy, DedicatedPoolSize>
{
public:
    /// 转发到 Derived::infer。
    template <typename T>
    decltype(auto) on_run(T &&input)
    {
        return static_cast<Derived *>(this)->infer(std::forward<T>(input));
    }

    /// const 重载：转发到 Derived::infer。
    template <typename T>
    decltype(auto) on_run(T &&input) const
    {
        return static_cast<const Derived *>(this)->infer(std::forward<T>(input));
    }
};

} // namespace NanoAI_FLOW
