// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: process_pipe.hpp
// Brief: 提供预处理/后处理 stage 的轻量适配器，统一把业务语义映射到 `on_run(...)` 契约。
//
// Design notes:
// - 该文件只做接口适配，不持有共享状态，也不绑定具体数据类型。
// - 通过 `preprocess/postprocess -> on_run` 的固定映射，减少业务 stage 的样板代码。

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
 *
 * 线程安全语义：
 * - 本基类不维护共享状态，线程安全由派生类 preprocess(...) 自行保证。
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
    /// 异常语义：透传派生类异常，不在此层吞掉错误。
    /// 设计意图：让预处理阶段代码使用更贴近业务语义的 `preprocess(...)` 命名。
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
 *
 * 线程安全语义：
 * - 与 PreprocessPipe 相同，不维护共享状态；并发安全由派生类负责。
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
    /// 异常语义：透传派生类异常，交由 pipeline 上层统一处理。
    /// 设计意图：把“后处理”语义从通用 `on_run(...)` 中显式分离出来，提升可读性。
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
