// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: pipeline.hpp
// Brief: 强类型并发流水线实现，支持阶段级调度、顺序转发与全局配额控制。

#pragma once

#include <algorithm>
#include <atomic>
#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

#include <BS_thread_pool.hpp>

#include "pipe.hpp"

namespace NanoAI_FLOW
{

    /**
     * @brief 元组样式类型约束。
     *
     * 用于检测某一阶段输出是否可按 tuple 语义展开。
     * 当上一阶段输出 tuple 时，流水线会把每个元素作为下一阶段的独立参数传入。
     */
    template <typename T>
    concept TupleLike = requires {
        typename std::tuple_size<std::remove_reference_t<T>>::type;
    };

    /**
     * @brief 强类型并发流水线。
     *
     * @tparam Pipes 阶段类型列表。每个类型都必须满足 PipeStage 约束。
     *
     * @par 并发模型总览
     * 本流水线面向“多生产者提交 + 多阶段并行处理 + 阶段间有序转发”的场景。
     * 每次 run(...) 调用都会分配一个全局递增序号 seq，随后按阶段递归执行。
     *
     * 关键目标：
     * 1) 每个阶段内部可以并行（受 stage 并发上限控制）。
     * 2) 全流水线总并发受全局配额限制，避免无限排队导致资源失控。
     * 3) 阶段间“转发顺序”稳定：同一阶段只允许按 seq 顺序把结果推给下一阶段。
     *
     * @par 请求生命周期（Request Lifecycle）
     * 对任意请求 R(seq) 而言，单阶段内的处理流程如下：
     * 1) 获取 StageParallelGuard：占用该阶段并发槽位（active_count < max_parallel）。
     * 2) 获取 GlobalQuotaGuard：占用全局令牌（global_tasks_in_use_ < global_task_quota_）。
     * 3) 按策略执行 dispatch：inline/shared_pool/dedicated_pool。
     * 4) 释放两类 guard（RAII 自动释放）。
     * 5) 进入顺序闸门：等待 seq == next_forward_seq，确保转发单调。
     * 6) 放行并 next_forward_seq++，唤醒后续等待者。
     *
     * @par 时序语义（Timing / Ordering）
     * - 计算阶段可以乱序完成（谁先算完谁先到顺序闸门）。
     * - 进入下一阶段前会按 seq 串行放行，因此“阶段边界上的输出顺序”是稳定的。
     * - 这意味着系统在高并发下兼顾吞吐（阶段内部并行）与可预测性（阶段间有序）。
     *
     * @par 锁粒度与锁竞争
     * - 阶段并发锁：StageState::parallel_mtx，仅保护 active_count。
     * - 阶段顺序锁：StageState::order_mtx，仅保护 next_forward_seq。
     * - 全局配额锁：NanoPipeLine::quota_mtx_，保护 global_tasks_in_use_。
     * 三类锁互相解耦，避免把不同维度的竞争集中到一把“大锁”上。
     *
     * @par 复杂度（忽略用户 on_run 算法成本）
     * - 单请求经过 N 个阶段，基础调度开销约为 O(N)。
     * - 每阶段有常数级锁操作与条件变量等待/唤醒。
     * - 当阶段处理时间远大于调度成本时，吞吐主要由并发度与线程池策略决定。
     */
    template <PipeStage... Pipes>
    class NanoPipeLine
    {
    public:
        /// 使用默认共享线程池大小与默认全局配额构造流水线。
        explicit NanoPipeLine(Pipes... pipes)
            : NanoPipeLine(0, 0, std::forward<Pipes>(pipes)...)
        {
        }

        /// 使用显式共享线程池大小与全局配额构造流水线。
        NanoPipeLine(nanoai_u32 shared_pool_size, nanoai_u32 global_task_quota, Pipes... pipes)
            : shared_pool_size_(resolve_pool_size(shared_pool_size)),
              shared_pool_(shared_pool_size_),
              global_task_quota_(resolve_quota(global_task_quota)),
              pipes_(std::forward<Pipes>(pipes)...),
              stages_(build_stages_from_tuple(this, pipes_))
        {
        }

        ~NanoPipeLine() = default;

        /// 获取共享线程池工作线程数。
        nanoai_u32 shared_pool_size() const noexcept
        {
            return shared_pool_size_;
        }

        /// 获取全局在途任务配额上限。
        nanoai_u32 global_task_quota() const noexcept
        {
            return global_task_quota_;
        }

        /// 运行时更新全局配额，最小钳制为 1。
        void set_global_task_quota(nanoai_u32 quota)
        {
            std::lock_guard lock(quota_mtx_);
            global_task_quota_ = std::max<nanoai_u32>(1, quota);
            quota_cv_.notify_all();
        }

        /// 当前流水线的阶段数量（编译期固定）。
        static constexpr nanoai_u32 pipe_count() noexcept
        {
            return static_cast<nanoai_u32>(sizeof...(Pipes));
        }

        template <typename P>
            requires PipeStage<std::remove_cvref_t<P>>
        /// 在当前流水线末尾追加一个阶段（左值版本）。
        auto add_pipe(P &&pipe) &
        {
            return add_pipe_impl(*this, std::forward<P>(pipe));
        }

        template <typename P>
            requires PipeStage<std::remove_cvref_t<P>>
        /// 在当前流水线末尾追加一个阶段（const 左值版本）。
        auto add_pipe(P &&pipe) const &
        {
            return add_pipe_impl(*this, std::forward<P>(pipe));
        }

        template <typename P>
            requires PipeStage<std::remove_cvref_t<P>>
        /// 在当前流水线末尾追加一个阶段（右值版本，支持链式转移）。
        auto add_pipe(P &&pipe) &&
        {
            return add_pipe_impl(std::move(*this), std::forward<P>(pipe));
        }

        /// 执行一次请求，让输入依次流经全部阶段。
        template <typename... Args>
        auto run(Args &&...args)
        {
            // seq 用于阶段间顺序闸门，保证转发单调。
            const auto seq = next_seq_.fetch_add(1, std::memory_order_relaxed);
            return run_chain<0>(seq, std::forward<Args>(args)...);
        }

        /// 便捷封装：将 run(...) 结果显式转换为 Out。
        template <typename Out, typename... Args>
        Out run_as(Args &&...args)
        {
            return static_cast<Out>(run(std::forward<Args>(args)...));
        }

    private:
        /**
         * @brief 单阶段运行时状态。
         * @tparam PipeT 阶段对象类型（可能是值类型，也可能是 std::reference_wrapper<Stage>）。
         */
        template <typename PipeT>
        struct StageState
        {
            explicit StageState(NanoPipeLine *owner_line, PipeT p)
                : owner(owner_line),
                  pipe(std::move(p)),
                  max_parallel(std::max<nanoai_u32>(1, access_pipe().concurrency())),
                  policy(access_pipe().execution_policy())
            {
                if (policy == PipeExecutionPolicy::dedicated_pool)
                {
                    dedicated_pool = std::make_unique<BS::thread_pool<>>(resolve_dedicated_pool_size());
                }
            }

            /// 处理该阶段的一次请求，并在尾部执行有序转发控制。
            template <typename... In>
            auto process(nanoai_u64 seq, In &&...input)
            {
                using Out = std::remove_cvref_t<decltype(dispatch(std::forward<In>(input)...))>;
                static_assert(!std::is_void_v<Out>, "Pipe::run(...) must return a value type.");

                std::exception_ptr error;
                std::optional<Out> output;

                {
                    StageParallelGuard parallel_guard(*this);
                    GlobalQuotaGuard quota_guard(*owner);
                    try
                    {
                        output.emplace(dispatch(std::forward<In>(input)...));
                    }
                    catch (...)
                    {
                        error = std::current_exception();
                    }
                }

                {
                    std::unique_lock order_lock(order_mtx);
                    order_cv.wait(order_lock, [this, seq]
                                  { return seq == next_forward_seq; });
                    // 仅允许“当前期待序号”前推到下一阶段。
                    ++next_forward_seq;
                }
                order_cv.notify_all();

                if (error)
                {
                    std::rethrow_exception(error);
                }
                return std::move(*output);
            }

            /// 回指所属流水线，用于访问共享线程池与全局配额。
            NanoPipeLine *owner;
            /// 阶段对象本体（值语义或 reference_wrapper）。
            PipeT pipe;
            /// 本阶段最大并发任务数。
            const nanoai_u32 max_parallel;
            /// 本阶段执行策略。
            const PipeExecutionPolicy policy;
            /// 专用线程池（仅 dedicated_pool 策略使用）。
            std::unique_ptr<BS::thread_pool<>> dedicated_pool;

            /// 阶段并发计数锁。
            std::mutex parallel_mtx;
            std::condition_variable parallel_cv;
            /// 当前阶段活跃任务数。
            nanoai_u32 active_count{0};

            /// 阶段内顺序转发锁。
            std::mutex order_mtx;
            std::condition_variable order_cv;
            /// 当前阶段下一次允许放行的 seq。
            nanoai_u64 next_forward_seq{0};

        private:
            /// RAII：进入阶段前申请并发槽，离开阶段后归还。
            struct StageParallelGuard
            {
                explicit StageParallelGuard(StageState &s) : stage(s)
                {
                    std::unique_lock lock(stage.parallel_mtx);
                    stage.parallel_cv.wait(lock, [&]
                                           { return stage.active_count < stage.max_parallel; });
                    ++stage.active_count;
                }

                ~StageParallelGuard()
                {
                    {
                        std::lock_guard lock(stage.parallel_mtx);
                        --stage.active_count;
                    }
                    stage.parallel_cv.notify_one();
                }

                StageState &stage;
            };

            /// RAII：申请与释放全局在途任务配额令牌。
            struct GlobalQuotaGuard
            {
                explicit GlobalQuotaGuard(NanoPipeLine &line) : owner(line)
                {
                    owner.acquire_global_task_token();
                }

                ~GlobalQuotaGuard()
                {
                    owner.release_global_task_token();
                }

                NanoPipeLine &owner;
            };

            /// 采集参数：左值转 std::ref，右值按值/移动捕获。
            static constexpr auto capture_input(auto &&value)
            {
                if constexpr (std::is_lvalue_reference_v<decltype(value)>)
                {
                    return std::ref(value);
                }
                else
                {
                    return std::forward<decltype(value)>(value);
                }
            }

            /// 采集全部输入到拥有所有权的 tuple，便于跨线程提交。
            static constexpr auto capture_inputs(auto &&...value)
            {
                return std::tuple<std::remove_cvref_t<decltype(capture_input(std::forward<decltype(value)>(value)))>...>(
                    capture_input(std::forward<decltype(value)>(value))...);
            }

            /// 从捕获形态恢复转发语义（ref_wrapper 解引用，值类型移动）。
            static constexpr decltype(auto) forward_captured(auto &value)
            {
                using Captured = std::remove_cvref_t<decltype(value)>;
                if constexpr (is_reference_wrapper<Captured>::value)
                {
                    return value.get();
                }
                else
                {
                    return std::move(value);
                }
            }

            /// 按执行策略分发调用。
            template <typename... In>
            auto dispatch(In &&...input)
            {
                switch (policy)
                {
                case PipeExecutionPolicy::inline_run:
                    return access_pipe().run(std::forward<In>(input)...);
                case PipeExecutionPolicy::dedicated_pool:
                {
                    auto captured = capture_inputs(std::forward<In>(input)...);
                    return dedicated_pool->submit_task([this, captured = std::move(captured)]() mutable
                                                       { return std::apply([this](auto &...args)
                                                                           { return access_pipe().run(forward_captured(args)...); },
                                                                           captured); })
                        .get();
                }
                case PipeExecutionPolicy::shared_pool:
                default:
                {
                    auto captured = capture_inputs(std::forward<In>(input)...);
                    return owner->shared_pool_.submit_task([this, captured = std::move(captured)]() mutable
                                                           { return std::apply([this](auto &...args)
                                                                               { return access_pipe().run(forward_captured(args)...); },
                                                                               captured); })
                        .get();
                }
                }
            }

            /// 解析专用线程池大小：优先 pipe 配置，缺省回退到并发度。
            nanoai_u32 resolve_dedicated_pool_size() const noexcept
            {
                const nanoai_u32 configured = access_pipe().dedicated_pool_size();
                if (configured != 0)
                {
                    return configured;
                }
                return std::max<nanoai_u32>(1, access_pipe().concurrency());
            }

            /// 访问辅助：必要时解包 std::reference_wrapper。
            static constexpr decltype(auto) unwrap_pipe(PipeT &p) noexcept
            {
                if constexpr (is_reference_wrapper<std::remove_cvref_t<PipeT>>::value)
                {
                    return p.get();
                }
                else
                {
                    return (p);
                }
            }

            /// const 访问辅助：必要时解包 std::reference_wrapper。
            static constexpr decltype(auto) unwrap_pipe(const PipeT &p) noexcept
            {
                if constexpr (is_reference_wrapper<std::remove_cvref_t<PipeT>>::value)
                {
                    return p.get();
                }
                else
                {
                    return (p);
                }
            }

            /// 返回可写阶段对象引用（自动解包 reference_wrapper）。
            constexpr decltype(auto) access_pipe() noexcept
            {
                return unwrap_pipe(pipe);
            }

            /// 返回只读阶段对象引用（自动解包 reference_wrapper）。
            constexpr decltype(auto) access_pipe() const noexcept
            {
                return unwrap_pipe(pipe);
            }
        };

        /// 根据传入阶段对象构建运行时 StageState 元组。
        template <typename... Args>
        static auto build_stages(NanoPipeLine *owner, Args &&...args)
        {
            return std::make_tuple(std::make_shared<StageState<Pipes>>(owner, std::forward<Args>(args))...);
        }

        /// 结合索引序列，从 pipes 元组提取元素并构建 StageState。
        template <typename Tuple, nanoai_usize... I>
        static auto build_stages_from_tuple_impl(NanoPipeLine *owner, Tuple &pipes, std::index_sequence<I...>)
        {
            return build_stages(owner, std::get<I>(pipes)...);
        }

        /// 从 pipes_ 元组构建全部阶段状态。
        template <typename Tuple>
        static auto build_stages_from_tuple(NanoPipeLine *owner, Tuple &pipes)
        {
            return build_stages_from_tuple_impl(owner, pipes, std::index_sequence_for<Pipes...>{});
        }

        /// 将末端多返回值打包为单值或 tuple。
        template <typename... Args>
        static auto pack_multi_return(Args &&...args)
        {
            if constexpr (sizeof...(Args) == 1)
            {
                return (std::forward<Args>(args), ...);
            }
            else
            {
                return std::tuple<std::remove_cvref_t<Args>...>(std::forward<Args>(args)...);
            }
        }

        /// 递归执行阶段链。
        template <nanoai_usize I, typename... Args>
        auto run_chain(nanoai_u64 seq, Args &&...args)
        {
            if constexpr (I == sizeof...(Pipes))
            {
                if constexpr (sizeof...(Args) == 0)
                {
                    return std::tuple<>{};
                }
                else
                {
                    return pack_multi_return(std::forward<Args>(args)...);
                }
            }
            else
            {
                auto out = std::get<I>(stages_)->process(seq, std::forward<Args>(args)...);
                return forward_to_next<I + 1>(seq, std::move(out));
            }
        }

        /// 把当前阶段输出转发到下一阶段；若输出为 tuple 则自动展开。
        template <nanoai_usize I, typename Out>
        auto forward_to_next(nanoai_u64 seq, Out &&out)
        {
            if constexpr (TupleLike<Out>)
            {
                return std::apply(
                    [this, seq](auto &&...items)
                    {
                        return run_chain<I>(seq, std::forward<decltype(items)>(items)...);
                    },
                    std::forward<Out>(out));
            }
            else
            {
                return run_chain<I>(seq, std::forward<Out>(out));
            }
        }

        /// 通过索引序列把旧流水线阶段复制到新流水线，并追加新阶段。
        template <typename Self, typename P, nanoai_usize... I>
        static auto add_pipe_impl_with_indices(Self &&self, P &&pipe, std::index_sequence<I...>)
        {
            return NanoPipeLine<Pipes..., std::decay_t<P>>(
                self.shared_pool_size_,
                self.global_task_quota_,
                std::get<I>(std::forward<Self>(self).pipes_)...,
                std::forward<P>(pipe));
        }

        /// add_pipe 的内部实现入口。
        template <typename Self, typename P>
        static auto add_pipe_impl(Self &&self, P &&pipe)
        {
            return add_pipe_impl_with_indices(
                std::forward<Self>(self),
                std::forward<P>(pipe),
                std::index_sequence_for<Pipes...>{});
        }

        /// 申请一个全局配额令牌；若达到上限则阻塞等待。
        void acquire_global_task_token()
        {
            std::unique_lock lock(quota_mtx_);
            quota_cv_.wait(lock, [this]
                           { return global_tasks_in_use_ < global_task_quota_; });
            ++global_tasks_in_use_;
        }

        /// 归还一个全局配额令牌。
        void release_global_task_token() noexcept
        {
            {
                std::lock_guard lock(quota_mtx_);
                --global_tasks_in_use_;
            }
            quota_cv_.notify_one();
        }

        /// 自动解析共享线程池大小（按 CPU 核心数回退）。
        static nanoai_u32 resolve_pool_size(nanoai_u32 requested) noexcept
        {
            if (requested != 0)
            {
                return requested;
            }
            const auto hw = std::thread::hardware_concurrency();
            return std::max<nanoai_u32>(1, static_cast<nanoai_u32>(hw == 0 ? 4 : hw));
        }

        /// 自动解析全局配额（按 CPU 核心数回退）。
        static nanoai_u32 resolve_quota(nanoai_u32 requested) noexcept
        {
            if (requested != 0)
            {
                return requested;
            }
            const auto hw = std::thread::hardware_concurrency();
            const nanoai_u32 base = static_cast<nanoai_u32>(hw == 0 ? 4 : hw);
            return std::max<nanoai_u32>(2, static_cast<nanoai_u32>(base * 2));
        }

        /// 共享线程池工作线程数。
        nanoai_u32 shared_pool_size_;
        /// shared_pool 策略阶段共用的线程池。
        BS::thread_pool<> shared_pool_;

        /// 全局配额同步原语。
        std::mutex quota_mtx_;
        std::condition_variable quota_cv_;
        /// 全流水线最大在途任务数。
        nanoai_u32 global_task_quota_;
        /// 当前在途任务数。
        nanoai_u32 global_tasks_in_use_{0};

        /// 全局单调递增序号生成器。
        std::atomic<nanoai_u64> next_seq_{0};
        /// 阶段对象存储（拥有语义）。
        std::tuple<Pipes...> pipes_;
        /// 与 pipes_ 对齐的阶段运行时状态。
        std::tuple<std::shared_ptr<StageState<Pipes>>...> stages_;
    };

    template <typename... Pipes>
    NanoPipeLine(Pipes...) -> NanoPipeLine<std::decay_t<Pipes>...>;

    template <typename... Pipes>
    NanoPipeLine(nanoai_u32, nanoai_u32, Pipes...) -> NanoPipeLine<std::decay_t<Pipes>...>;

    template <typename BuildFn>
    class NanoPipeLineBuilder
    {
    public:
        /// @tparam BuildFn 延迟构建闭包类型。
        /// Builder 内部保存延迟构建闭包，直到 build() 才真正实例化流水线。
        explicit NanoPipeLineBuilder(BuildFn fn)
            : build_fn_(std::move(fn))
        {
        }

        template <typename P>
            requires PipeStage<std::remove_cvref_t<P>>
        auto add_pipe(P &&pipe) &
        {
            // 仅扩展构建链，不立即创建流水线对象。
            auto prev_fn = build_fn_;
            auto next_fn = [prev_fn = std::move(prev_fn), pipe = std::forward<P>(pipe)]() mutable
            { return prev_fn().add_pipe(std::move(pipe)); };
            return NanoPipeLineBuilder<std::decay_t<decltype(next_fn)>>(std::move(next_fn));
        }

        template <typename P>
            requires PipeStage<std::remove_cvref_t<P>>
        auto add_pipe(P &&pipe) const &
        {
            auto prev_fn = build_fn_;
            auto next_fn = [prev_fn = std::move(prev_fn), pipe = std::forward<P>(pipe)]() mutable
            { return prev_fn().add_pipe(std::move(pipe)); };
            return NanoPipeLineBuilder<std::decay_t<decltype(next_fn)>>(std::move(next_fn));
        }

        template <typename P>
            requires PipeStage<std::remove_cvref_t<P>>
        auto add_pipe(P &&pipe) &&
        {
            auto next_fn = [prev_fn = std::move(build_fn_), pipe = std::forward<P>(pipe)]() mutable
            { return prev_fn().add_pipe(std::move(pipe)); };
            return NanoPipeLineBuilder<std::decay_t<decltype(next_fn)>>(std::move(next_fn));
        }

        /// 从右值 builder 物化流水线。
        decltype(auto) build() &&
        {
            return std::move(build_fn_)();
        }

        /// 从 const 左值 builder 物化流水线。
        decltype(auto) build() const &
        {
            return build_fn_();
        }

    private:
        /// 延迟构建闭包。
        BuildFn build_fn_;
    };

    /// 创建流水线 builder 的工厂入口。
    inline auto make_pipeline_builder(nanoai_u32 shared_pool_size = 0, nanoai_u32 global_task_quota = 0)
    {
        return NanoPipeLineBuilder([shared_pool_size, global_task_quota]()
                                   { return NanoPipeLine<>(shared_pool_size, global_task_quota); });
    }

    /// 兼容历史命名的别名入口。
    inline auto NanoPipeBuilder(nanoai_u32 shared_pool_size = 0, nanoai_u32 global_task_quota = 0)
    {
        return make_pipeline_builder(shared_pool_size, global_task_quota);
    }

    template <typename Fn>
    NanoPipeLineBuilder(Fn) -> NanoPipeLineBuilder<std::decay_t<Fn>>;

} // namespace NanoAI_FLOW
