// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: pipe.hpp
// Brief: 定义基于 CRTP 的阶段基类与流水线阶段约束。
//
// 该文件给出“stage 接口最小契约”：
// - 通过 PipeStage 概念约束阶段必须提供的元信息接口；
// - 通过 NanoPipe<Derived,...> 提供统一 run() 分发与策略参数化。

#pragma once


#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>
#include "types.h"
#include "nanoai_flow/core/cancel_token.hpp"

namespace NanoAI_FLOW
{

/**
 * @brief 定义阶段执行位置策略。
 */
enum class PipeExecutionPolicy
{
    /// 在流水线共享线程池执行。
    shared_pool,
    /// 在阶段私有线程池执行。
    dedicated_pool,
    /// 在调用线程内联执行。
    inline_run
};

/// 类型萃取：仅当 T 为 std::reference_wrapper<U> 时为 true。
template <typename T>
struct is_reference_wrapper : std::false_type
{
};

/// std::reference_wrapper 特化。
template <typename U>
struct is_reference_wrapper<std::reference_wrapper<U>> : std::true_type
{
};

/// 工具萃取：去除 cv/ref，保留普通值语义类型。
template <typename T>
struct unwrap_reference_wrapper
{
    using type = std::remove_cvref_t<T>;
};

/// 工具萃取特化：把 std::reference_wrapper<U> 解包为 U。
template <typename U>
struct unwrap_reference_wrapper<std::reference_wrapper<U>>
{
    using type = U;
};

/// unwrap_reference_wrapper 的便捷别名。
template <typename T>
using unwrap_reference_wrapper_t = typename unwrap_reference_wrapper<std::remove_cvref_t<T>>::type;

/**
 * @brief 流水线阶段概念约束。
 *
 * 阶段必须暴露并发与策略元信息，以便调度器安全调度。
 */
template <typename T>
concept PipeStage = requires(unwrap_reference_wrapper_t<T> &pipe)
{
    { pipe.concurrency() } -> std::convertible_to<nanoai_u32>;
    { pipe.execution_policy() } -> std::same_as<PipeExecutionPolicy>;
    { pipe.dedicated_pool_size() } -> std::convertible_to<nanoai_u32>;
};

/**
 * @brief 管线 stage 的 CRTP 基类。
 *
 * 设计目标：
 * - 让每个 stage 在编译期声明并发度与执行策略；
 * - 统一向 pipeline 暴露 run()/concurrency()/execution_policy() 接口；
 * - 避免虚函数开销，保持零成本抽象风格。
 */
template <
    typename Derived,
    nanoai_u32 MaxConcurrency = 1,
    PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
    nanoai_u32 DedicatedPoolSize = 0>
class NanoPipe
{
    /// 将 0 规范化为 1，避免出现“无并发槽位”的非法配置。
    static constexpr nanoai_u32 normalized_concurrency = MaxConcurrency == 0 ? 1 : MaxConcurrency;

public:
    /**
     * @brief CRTP 阶段基类。
     * @tparam Derived 具体派生阶段类型。
     * @tparam MaxConcurrency 阶段最大并发数，0 会被规范化为 1。
     * @tparam Policy 执行策略。
     * @tparam DedicatedPoolSize 专用线程池大小（仅 dedicated_pool 有效）。
     */
    constexpr NanoPipe() noexcept = default;
    ~NanoPipe() = default;

    /// 返回该阶段允许的最大并发任务数。
    constexpr nanoai_u32 concurrency() const noexcept
    {
        return normalized_concurrency;
    }

    /// 返回该阶段执行策略。
    constexpr PipeExecutionPolicy execution_policy() const noexcept
    {
        return Policy;
    }

    /// 返回 dedicated_pool 模式下的有效线程数。
    constexpr nanoai_u32 dedicated_pool_size() const noexcept
    {
        // 仅 dedicated_pool 策略下该值有效。
        if constexpr (Policy == PipeExecutionPolicy::dedicated_pool)
        {
            if constexpr (DedicatedPoolSize > 0)
            {
                return DedicatedPoolSize;
            }
            return normalized_concurrency;
        }
        return 0;
    }

    /// 非 const 调度入口，转发到 Derived::on_run(...)
    template <typename... Args>
    decltype(auto) run(Args &&...args)
    {
        return static_cast<Derived *>(this)->on_run(std::forward<Args>(args)...);
    }

    /// const 调度入口，转发到 Derived::on_run(...)
    template <typename... Args>
    decltype(auto) run(Args &&...args) const
    {
        return static_cast<const Derived *>(this)->on_run(std::forward<Args>(args)...);
    }

    /**
     * @brief 支持协作式取消的 run 调度入口。
     *
     * 行为：
     * - 若派生类 `on_run(...)` 显式接受 `NanoCancelToken`，则把 token 追加传入；
     * - 否则自动回退到旧签名，保证老 stage 不因引入取消机制而被迫修改。
     *
     * 设计意图：把“是否感知取消”交给具体 stage 决定，而不是强制所有 stage
     * 都接受 token 参数，这样可以平滑兼容纯计算型或无阻塞阶段。
     */
    template <typename... Args>
    decltype(auto) run_with_cancel(const NanoCancelToken &cancel_token, Args &&...args)
    {
        if constexpr (requires(Derived &d, Args &&...a, const NanoCancelToken &t) { d.on_run(std::forward<Args>(a)..., t); })
        {
            return static_cast<Derived *>(this)->on_run(std::forward<Args>(args)..., cancel_token);
        }
        else
        {
            return static_cast<Derived *>(this)->on_run(std::forward<Args>(args)...);
        }
    }

    /// const 版本行为与非 const 相同，只是转发到 `const Derived::on_run(...)`。
    template <typename... Args>
    decltype(auto) run_with_cancel(const NanoCancelToken &cancel_token, Args &&...args) const
    {
        if constexpr (requires(const Derived &d, Args &&...a, const NanoCancelToken &t) { d.on_run(std::forward<Args>(a)..., t); })
        {
            return static_cast<const Derived *>(this)->on_run(std::forward<Args>(args)..., cancel_token);
        }
        else
        {
            return static_cast<const Derived *>(this)->on_run(std::forward<Args>(args)...);
        }
    }
};

} // namespace NanoAI_FLOW
