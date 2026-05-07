// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: process_pipe.hpp
// Brief: 预处理与后处理阶段通用适配器。

#pragma once

#include "../core/pipe.hpp"
#include <utility>

namespace NanoAI_FLOW
{

/**
 * @brief 预处理阶段适配器基类。
 * @tparam Derived 具体派生阶段类型，需要实现 preprocess(...)
 * @tparam NumThreads 阶段并发度配置。
 * @tparam Policy 阶段执行策略。
 * @tparam DedicatedPoolSize 专用线程池大小（仅 dedicated_pool 生效）。
 */
template <
    typename Derived,
    nanoai_u32 NumThreads = 0,
    PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
    nanoai_u32 DedicatedPoolSize = 0>
class PreprocessPipe : public NanoPipe<Derived, NumThreads, Policy, DedicatedPoolSize>
{
public:
    /// 转发到 Derived::preprocess。
    template <typename T>
    decltype(auto) on_run(T &&input)
    {
        return static_cast<Derived *>(this)->preprocess(std::forward<T>(input));
    }

    /// const 重载：转发到 Derived::preprocess。
    template <typename T>
    decltype(auto) on_run(T &&input) const
    {
        return static_cast<const Derived *>(this)->preprocess(std::forward<T>(input));
    }
};

/**
 * @brief 后处理阶段适配器基类。
 * @tparam Derived 具体派生阶段类型，需要实现 postprocess(...)
 * @tparam NumThreads 阶段并发度配置。
 * @tparam Policy 阶段执行策略。
 * @tparam DedicatedPoolSize 专用线程池大小（仅 dedicated_pool 生效）。
 */
template <
    typename Derived,
    nanoai_u32 NumThreads = 0,
    PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
    nanoai_u32 DedicatedPoolSize = 0>
class PostProcessPipe : public NanoPipe<Derived, NumThreads, Policy, DedicatedPoolSize>
{
public:
    /// 转发到 Derived::postprocess。
    template <typename T>
    decltype(auto) on_run(T &&input)
    {
        return static_cast<Derived *>(this)->postprocess(std::forward<T>(input));
    }

    /// const 重载：转发到 Derived::postprocess。
    template <typename T>
    decltype(auto) on_run(T &&input) const
    {
        return static_cast<const Derived *>(this)->postprocess(std::forward<T>(input));
    }
};

} // namespace NanoAI_FLOW
