// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: pipeline.hpp
// Brief: 编译期特化并发流水线实现（固定 ordered/unordered）。
//
// Core goals:
// - 使用强类型模板在编译期固定管线拓扑，避免运行时虚调度开销。
// - 同时支持 shared pool / dedicated pool / inline 三种执行策略。
// - 通过 sequence + waiters 机制在 ordered 模式下保证输出顺序一致。
// - 提供可拷贝/可移动构造语义，便于将 pipeline 作为值对象传递。

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <new>
#include <optional>
#include <stdexcept>
#include <semaphore>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "nanoai_flow/core/cancel_token.hpp"

#include "pipe.hpp"
#include "thread_pool.hpp"

namespace NanoAI_FLOW
{

    // 数据在 stage 间的转发顺序策略。
    // ordered: 严格按输入序号转发。
    // unordered: 谁先完成谁先转发，追求吞吐优先。
    enum class PipeForwardOrder
    {
        ordered,
        unordered
    };

    // NanoPipeLine 全局可调参数。
    // 这些选项用于控制线程资源与 ordered drain 行为。
    struct NanoPipeLineOptions
    {
        // 共享线程池大小。0 表示按硬件并发自动推导。
        nanoai_u32 shared_pool_size{0};
        // 全局并行任务配额。0 表示自动推导。
        nanoai_u32 global_task_quota{0};

        // Ordered forwarding adaptive drain controls.
        // 仅在 ordered 模式下用于控制“积压 action”分批执行策略。
        // min/max: 批次下界和上界。
        // fast/slow threshold: 一批执行耗时的快慢阈值（微秒）。
        // yield_on_*: 是否在特定条件下主动让出 CPU，改善系统公平性。
        nanoai_u32 drain_batch_min{8};
        nanoai_u32 drain_batch_max{512};
        nanoai_u32 drain_fast_threshold_us{80};
        nanoai_u32 drain_slow_threshold_us{300};
        bool yield_on_full_batch{true};
        bool yield_on_slow_batch{true};

        // 共享线程池队列上限（0 表示自动推导）。
        nanoai_usize shared_pool_queue_capacity{0};
        // 共享线程池提交策略。
        ThreadPoolSubmitPolicy shared_pool_submit_policy{ThreadPoolSubmitPolicy::block};
        // 共享线程池 timeout 提交等待时长（毫秒）。
        nanoai_u32 shared_pool_submit_timeout_ms{1000};

        // 专用线程池队列上限（0 表示自动推导）。
        nanoai_usize dedicated_pool_queue_capacity{0};
        // 专用线程池提交策略。
        ThreadPoolSubmitPolicy dedicated_pool_submit_policy{ThreadPoolSubmitPolicy::block};
        // 专用线程池 timeout 提交等待时长（毫秒）。
        nanoai_u32 dedicated_pool_submit_timeout_ms{1000};

        // ordered 模式下 waiters 最大积压（0 表示不限制）。
        nanoai_usize ordered_waiters_limit{0};
        // ordered 模式单次 dispatch 预算（0 表示不限制）。
        nanoai_usize ordered_dispatch_budget{0};

        // run() 同步等待超时（毫秒），0 表示不超时。
        nanoai_u32 run_timeout_ms{0};

        // 运行时事件回调（可选）。
        // 参数：event_name, sequence。
        std::function<void(const char *, nanoai_u64)> event_callback{};
    };

    // NanoPipeLine 运行时统计快照。
    // 线程安全语义：
    // - stats() 会从原子计数和受锁状态汇总生成该结构体；
    // - 该结构体本身是值快照，不会随运行态自动更新。
    struct NanoPipeLineStats
    {
        // run() 被接受提交的次数。
        nanoai_u64 run_submitted{0};
        // run() 完成收敛（成功或失败）的次数。
        nanoai_u64 run_completed{0};
        // run() 因 shutdown/拒绝策略未被接受的次数。
        nanoai_u64 run_rejected{0};
        // run() 因超时退出的次数。
        nanoai_u64 run_timeout{0};
        // stage 异步分发到线程池失败的次数。
        nanoai_u64 dispatch_rejected{0};
        // ordered 模式等待队列溢出的次数。
        nanoai_u64 ordered_waiter_overflow{0};
        // 当前在途任务数（受 shutdown 锁保护统计）。
        nanoai_u64 in_flight_tasks{0};
        // 下一个将被分配的全局序号。
        nanoai_u64 next_sequence{0};
    };

    // 判断某类型是否可被视为 tuple-like，用于处理多返回值转发。
    template <typename T>
    concept TupleLike = requires {
        typename std::tuple_size<std::remove_reference_t<T>>::type;
    };

    // NanoPipeLine:
    // - FixedOrder 在编译期确定，避免运行时分支传播。
    // - Pipes... 是各 stage 的类型序列。
    // - run() 是同步 API，内部借助异步链 + 信号量等待实现。
    template <PipeForwardOrder FixedOrder, PipeStage... Pipes>
    class NanoPipeLine
    {
    public:
        // 与 semaphore 模板参数一致的许可上限。
        static constexpr std::ptrdiff_t kMaxSemaphorePermits = 1024 * 1024;

        // 默认构造：使用自动推导的线程池/配额策略。
        explicit NanoPipeLine(Pipes... pipes)
            : NanoPipeLine(NanoPipeLineOptions{}, std::forward<Pipes>(pipes)...)
        {
        }

        // 兼容旧接口：直接传入 shared_pool_size 和 global_task_quota。
        NanoPipeLine(nanoai_u32 shared_pool_size, nanoai_u32 global_task_quota, Pipes... pipes)
            : NanoPipeLine(NanoPipeLineOptions{shared_pool_size, global_task_quota}, std::forward<Pipes>(pipes)...)
        {
        }

        // 完整构造：归一化配置、构建共享池、并为每个 pipe 建立 stage 状态。
        NanoPipeLine(NanoPipeLineOptions options, Pipes... pipes)
            : options_(normalize_options(options)),
              shared_pool_size_(resolve_pool_size(options_.shared_pool_size)),
                            shared_pool_(NanoThreadPoolOptions{
                                    .thread_count = shared_pool_size_,
                                    .max_queue_size = options_.shared_pool_queue_capacity,
                                    .submit_policy = options_.shared_pool_submit_policy,
                                    .submit_timeout_ms = options_.shared_pool_submit_timeout_ms}),
              global_task_quota_(resolve_quota(options_.global_task_quota)),
              global_quota_sem_(static_cast<std::ptrdiff_t>(global_task_quota_)),
              pipes_(std::forward<Pipes>(pipes)...),
              stages_(build_stages_from_tuple(this, pipes_))
        {
        }

                // 拷贝构造语义：复制 pipe 拓扑与配置，但运行时状态（线程、序号、队列）重新初始化。
                NanoPipeLine(const NanoPipeLine &other)
                        : options_(other.options_),
                            shared_pool_size_(other.shared_pool_size_),
                            shared_pool_(NanoThreadPoolOptions{
                                .thread_count = shared_pool_size_,
                                .max_queue_size = options_.shared_pool_queue_capacity,
                                .submit_policy = options_.shared_pool_submit_policy,
                                .submit_timeout_ms = options_.shared_pool_submit_timeout_ms}),
                            global_task_quota_(other.global_task_quota_),
                            global_quota_sem_(static_cast<std::ptrdiff_t>(global_task_quota_)),
                            next_seq_(0),
                            pipes_(other.pipes_),
                            stages_(build_stages_from_tuple(this, pipes_))
                {
                }

                // 移动构造语义：移动 pipe 对象本身，运行时状态仍采用全新实例。
                NanoPipeLine(NanoPipeLine &&other)
                        : options_(other.options_),
                            shared_pool_size_(other.shared_pool_size_),
                            shared_pool_(NanoThreadPoolOptions{
                                .thread_count = shared_pool_size_,
                                .max_queue_size = options_.shared_pool_queue_capacity,
                                .submit_policy = options_.shared_pool_submit_policy,
                                .submit_timeout_ms = options_.shared_pool_submit_timeout_ms}),
                            global_task_quota_(other.global_task_quota_),
                            global_quota_sem_(static_cast<std::ptrdiff_t>(global_task_quota_)),
                            next_seq_(0),
                            pipes_(std::move(other.pipes_)),
                            stages_(build_stages_from_tuple(this, pipes_))
                {
                }

                NanoPipeLine &operator=(const NanoPipeLine &) = delete;
                NanoPipeLine &operator=(NanoPipeLine &&) = delete;

        ~NanoPipeLine()
        {
            shutdown();
        }

        // 显式关闭流水线：
        // 1) 拒绝后续新任务提交；
        // 2) 等待所有已提交在途任务完成；
        // 3) 返回后可安全析构运行时状态。
        void shutdown()
        {
            std::unique_lock<std::mutex> lock(shutdown_mtx_);
            accepting_tasks_ = false;
            if (global_cancel_token_) {
                global_cancel_token_->request_cancel();
            }
            shutdown_cv_.wait(lock, [this] {
                return in_flight_tasks_ == 0;
            });
        }

        // 带超时的关闭接口。
        // 输入：timeout 为最长等待时长。
        // 返回：true 表示在 timeout 内所有在途任务完成；false 表示超时未收敛。
        // 并发：调用后立即拒绝新任务，并触发协作式取消请求。
        [[nodiscard]] bool shutdown_for(std::chrono::milliseconds timeout)
        {
            std::unique_lock<std::mutex> lock(shutdown_mtx_);
            accepting_tasks_ = false;
            if (global_cancel_token_)
            {
                global_cancel_token_->request_cancel();
            }
            return shutdown_cv_.wait_for(lock, timeout, [this] {
                return in_flight_tasks_ == 0;
            });
        }

        // 主动请求取消所有在途任务（协作式，不强杀线程）。
        // 说明：该接口只发出取消信号，不等待任务退出。
        void request_cancel_all()
        {
            if (global_cancel_token_)
            {
                global_cancel_token_->request_cancel();
            }
        }

        // 查询当前是否仍接受新 run()/dispatch 提交。
        // 返回：true 表示允许新任务进入；false 表示处于 shutdown 后拒绝态。
        [[nodiscard]] bool is_accepting_tasks() const noexcept
        {
            std::lock_guard<std::mutex> lock(shutdown_mtx_);
            return accepting_tasks_;
        }

        // 触发观测事件回调。
        // 输入：name 为事件名，seq 为相关任务序号（无序号时可传 0）。
        // 异常：内部吞掉回调异常，保证观测路径不影响主流程。
        void emit_event(const char *name, nanoai_u64 seq) noexcept
        {
            auto cb = options_.event_callback;
            if (!cb)
            {
                return;
            }

            try
            {
                cb(name, seq);
            }
            catch (...)
            {
                // 观测回调不可影响主流程。
            }
        }

        // 读取运行统计快照。
        // 返回：当前时刻统计值，不保证与后续执行保持同步。
        [[nodiscard]] NanoPipeLineStats stats() const noexcept
        {
            NanoPipeLineStats s;
            s.run_submitted = run_submitted_count_.load(std::memory_order_relaxed);
            s.run_completed = run_completed_count_.load(std::memory_order_relaxed);
            s.run_rejected = run_rejected_count_.load(std::memory_order_relaxed);
            s.run_timeout = run_timeout_count_.load(std::memory_order_relaxed);
            s.dispatch_rejected = dispatch_rejected_count_.load(std::memory_order_relaxed);
            s.ordered_waiter_overflow = ordered_waiter_overflow_count_.load(std::memory_order_relaxed);
            s.next_sequence = next_seq_.load(std::memory_order_relaxed);
            {
                std::lock_guard<std::mutex> lock(shutdown_mtx_);
                s.in_flight_tasks = in_flight_tasks_;
            }
            return s;
        }

        // 返回编译期固定的转发顺序策略。
        static constexpr PipeForwardOrder fixed_forward_order() noexcept
        {
            return FixedOrder;
        }

        // 共享线程池大小（归一化后的实际值）。
        nanoai_u32 shared_pool_size() const noexcept
        {
            return shared_pool_size_;
        }

        // 全局任务配额（归一化后的实际值）。
        nanoai_u32 global_task_quota() const noexcept
        {
            return global_task_quota_;
        }

        // 编译期 stage 数量。
        static constexpr nanoai_u32 pipe_count() noexcept
        {
            return static_cast<nanoai_u32>(sizeof...(Pipes));
        }

        // 同步执行入口：
        // 1) 分配全局 sequence id；
        // 2) 触发异步链；
        // 3) 用 binary_semaphore 等待收敛；
        // 4) 透传异常或返回结果。
        // 异常：
        // - shutdown 后提交会抛 runtime_error；
        // - run_timeout_ms 命中会抛 runtime_error；
        // - stage 执行异常按原始异常透传。
        template <typename... Args>
        auto run(Args &&...args)
        {
            if (!on_task_submitted_if_accepting())
            {
                run_rejected_count_.fetch_add(1, std::memory_order_acq_rel);
                emit_event("run_rejected", 0);
                throw std::runtime_error("NanoPipeLine is shutting down and no longer accepts new runs");
            }

            run_submitted_count_.fetch_add(1, std::memory_order_acq_rel);

            struct RunGuard
            {
                explicit RunGuard(NanoPipeLine &line) : owner(line) {}
                ~RunGuard()
                {
                    owner.run_completed_count_.fetch_add(1, std::memory_order_acq_rel);
                    owner.on_task_finished();
                }
                NanoPipeLine &owner;
            } run_guard(*this);

            const auto seq = next_seq_.fetch_add(1, std::memory_order_relaxed);
            using Return = decltype(this->template run_chain<0>(seq, std::forward<Args>(args)...));

            std::binary_semaphore done_sem{0};
            std::exception_ptr error;
            std::optional<Return> output;

            run_chain_async<0>(
                seq,
                [&](auto &&value) {
                    output.emplace(std::forward<decltype(value)>(value));
                    done_sem.release();
                },
                [&](std::exception_ptr e) {
                    error = e;
                    done_sem.release();
                },
                std::forward<Args>(args)...);

            if (options_.run_timeout_ms == 0)
            {
                done_sem.acquire();
            }
            else if (!done_sem.try_acquire_for(std::chrono::milliseconds(options_.run_timeout_ms)))
            {
                run_timeout_count_.fetch_add(1, std::memory_order_acq_rel);
                emit_event("run_timeout", seq);
                throw std::runtime_error("NanoPipeLine run timeout");
            }

            if (error)
            {
                std::rethrow_exception(error);
            }
            return std::move(*output);
        }

        // 协作式取消版本的同步执行入口。
        // 输入：cancel_token 为外部取消信号，args 为业务输入。
        // 行为：
        // - 提交前若 token 已取消，立即失败；
        // - 执行期间若 token 被取消，run_with_cancel 在收敛点返回取消异常。
        // 说明：协作式取消不强杀 worker，具体中断时机取决于各 stage 检查 token 的频率。
        template <typename... Args>
        auto run_with_cancel(const NanoCancelToken& cancel_token, Args&&... args)
        {
            if (cancel_token.is_cancelled()) {
                throw std::runtime_error("run cancelled before start");
            }
            if (!on_task_submitted_if_accepting()) {
                run_rejected_count_.fetch_add(1, std::memory_order_acq_rel);
                emit_event("run_rejected", 0);
                throw std::runtime_error("NanoPipeLine is shutting down and no longer accepts new runs");
            }
            run_submitted_count_.fetch_add(1, std::memory_order_acq_rel);
            struct RunGuard {
                explicit RunGuard(NanoPipeLine& line) : owner(line) {}
                ~RunGuard() {
                    owner.run_completed_count_.fetch_add(1, std::memory_order_acq_rel);
                    owner.on_task_finished();
                }
                NanoPipeLine& owner;
            } run_guard(*this);
            const auto seq = next_seq_.fetch_add(1, std::memory_order_relaxed);
            using Return = decltype(this->template run_chain<0>(seq, std::forward<Args>(args)...));
            std::binary_semaphore done_sem{0};
            std::exception_ptr error;
            std::optional<Return> output;
            run_chain_async_with_cancel<0>(
                seq,
                cancel_token,
                [&](auto&& value) {
                    output.emplace(std::forward<decltype(value)>(value));
                    done_sem.release();
                },
                [&](std::exception_ptr e) {
                    error = e;
                    done_sem.release();
                },
                std::forward<Args>(args)...);
            if (options_.run_timeout_ms == 0) {
                if (!cancel_token.is_cancelled()) {
                    done_sem.acquire();
                }
            } else {
                if (!done_sem.try_acquire_for(std::chrono::milliseconds(options_.run_timeout_ms))) {
                    run_timeout_count_.fetch_add(1, std::memory_order_acq_rel);
                    emit_event("run_timeout", seq);
                    throw std::runtime_error("NanoPipeLine run timeout");
                }
            }
            if (cancel_token.is_cancelled()) {
                throw std::runtime_error("NanoPipeLine run cancelled");
            }
            if (error) {
                std::rethrow_exception(error);
            }
            return std::move(*output);
        }

    private:
        // 每个 stage 对应一个 StageState，封装并发控制、转发顺序和执行策略。
        template <typename PipeT>
        struct StageState
        {
            // owner_line: 所属 pipeline。
            // p: 当前 stage 的 pipe 对象（或 reference_wrapper）。
            explicit StageState(NanoPipeLine *owner_line, PipeT p)
                : owner(owner_line),
                  pipe(std::move(p)),
                  max_parallel(std::max<nanoai_u32>(1, access_pipe().concurrency())),
                  policy(access_pipe().execution_policy()),
                  parallel_sem(static_cast<std::ptrdiff_t>(max_parallel)),
                  drain_batch_min(owner->options_.drain_batch_min),
                  drain_batch_max(std::max(owner->options_.drain_batch_min, owner->options_.drain_batch_max)),
                  drain_fast_threshold(std::chrono::microseconds(owner->options_.drain_fast_threshold_us)),
                  drain_slow_threshold(std::chrono::microseconds(owner->options_.drain_slow_threshold_us)),
                  yield_on_full_batch(owner->options_.yield_on_full_batch),
                  yield_on_slow_batch(owner->options_.yield_on_slow_batch),
                  drain_batch_target_(std::clamp<nanoai_u32>(64, owner->options_.drain_batch_min, std::max(owner->options_.drain_batch_min, owner->options_.drain_batch_max))),
                  order_waiters(&order_waiters_resource)
            {
                // dedicated_pool 策略仅在该 stage 上按需初始化专用线程池。
                if (policy == PipeExecutionPolicy::dedicated_pool)
                {
                    dedicated_pool = std::make_unique<NanoThreadPool>(
                        NanoThreadPoolOptions{
                            .thread_count = resolve_dedicated_pool_size(),
                            .max_queue_size = owner->options_.dedicated_pool_queue_capacity,
                            .submit_policy = owner->options_.dedicated_pool_submit_policy,
                            .submit_timeout_ms = owner->options_.dedicated_pool_submit_timeout_ms});
                }
            }

            // 异步执行当前 stage，并在完成后将结果以 action 形式交给顺序转发层。
            template <typename OnSuccess, typename OnError, typename... In>
            void process_async(nanoai_u64 seq, OnSuccess &&on_success, OnError &&on_error, In &&...input)
            {
                if (!owner->on_task_submitted_if_accepting())
                {
                    owner->dispatch_rejected_count_.fetch_add(1, std::memory_order_acq_rel);
                    owner->emit_event("dispatch_rejected_shutdown", seq);
                    safe_call_error(on_error, std::make_exception_ptr(std::runtime_error("NanoPipeLine is shutting down and no longer accepts new tasks")));
                    return;
                }

                // 捕获输入参数：lvalue 保持引用语义，rvalue 采用移动语义。
                auto captured = capture_inputs(std::forward<In>(input)...);
                auto task = [this,
                             seq,
                             captured = std::move(captured),
                             on_success = std::forward<OnSuccess>(on_success),
                             on_error = std::forward<OnError>(on_error)]() mutable {
                    struct InFlightGuard
                    {
                        explicit InFlightGuard(NanoPipeLine &line) : owner(line) {}
                        ~InFlightGuard()
                        {
                            owner.on_task_finished();
                        }
                        NanoPipeLine &owner;
                    } guard(*owner);

                    using Out = std::remove_cvref_t<decltype(std::apply(
                        [this](auto &...args)
                        { return access_pipe().run(forward_captured(args)...); },
                        captured))>;

                    std::exception_ptr error;
                    std::optional<Out> output;

                    {
                        // 双重限流：
                        // - stage 并行度上限（parallel_sem）
                        // - pipeline 全局任务配额（global_quota_sem_）
                        StageParallelGuard parallel_guard(*this);
                        GlobalQuotaGuard quota_guard(*owner);
                        try
                        {
                            output.emplace(std::apply(
                                [this](auto &...args)
                                { return access_pipe().run(forward_captured(args)...); },
                                captured));
                        }
                        catch (...)
                        {
                            error = std::current_exception();
                        }
                    }

                    // 将完成回调封装为可排序 action，统一进入 forward_in_order。
                    auto action = make_forward_action(
                        [on_success = std::move(on_success),
                         on_error = std::move(on_error),
                         error = std::move(error),
                         output = std::move(output)]() mutable {
                            if (error)
                            {
                                safe_call_error(on_error, error);
                                return;
                            }

                            try
                            {
                                on_success(std::move(*output));
                            }
                            catch (...)
                            {
                                safe_call_error(on_error, std::current_exception());
                            }
                        });

                    try
                    {
                        forward_in_order(seq, std::move(action));
                    }
                    catch (...)
                    {
                        safe_call_error(on_error, std::current_exception());
                    }
                };

                try
                {
                    dispatch_async(std::move(task));
                }
                catch (...)
                {
                    owner->on_task_finished();
                    safe_call_error(on_error, std::current_exception());
                }
            }

            // 同步封装：基于 process_async + binary_semaphore 构建阻塞等待接口。
            template <typename... In>
            auto process(nanoai_u64 seq, In &&...input)
            {
                using Out = std::remove_cvref_t<decltype(access_pipe().run(std::forward<In>(input)...))>;

                std::binary_semaphore done_sem{0};
                std::optional<Out> output;
                std::exception_ptr error;

                process_async(
                    seq,
                    [&](Out &&value) {
                        output.emplace(std::move(value));
                        done_sem.release();
                    },
                    [&](std::exception_ptr e) {
                        error = e;
                        done_sem.release();
                    },
                    std::forward<In>(input)...);

                done_sem.acquire();

                if (error)
                {
                    std::rethrow_exception(error);
                }
                return std::move(*output);
            }

            // 异步执行（协作式取消版本）。
            template <typename OnSuccess, typename OnError, typename... In>
            void process_async(nanoai_u64 seq, const NanoCancelToken &cancel_token, OnSuccess &&on_success, OnError &&on_error, In &&...input)
            {
                if (cancel_token.is_cancelled())
                {
                    safe_call_error(on_error, std::make_exception_ptr(std::runtime_error("NanoPipeLine run cancelled")));
                    return;
                }

                if (!owner->on_task_submitted_if_accepting())
                {
                    owner->dispatch_rejected_count_.fetch_add(1, std::memory_order_acq_rel);
                    owner->emit_event("dispatch_rejected_shutdown", seq);
                    safe_call_error(on_error, std::make_exception_ptr(std::runtime_error("NanoPipeLine is shutting down and no longer accepts new tasks")));
                    return;
                }

                auto captured = capture_inputs(std::forward<In>(input)...);
                auto task = [this,
                             seq,
                             &cancel_token,
                             captured = std::move(captured),
                             on_success = std::forward<OnSuccess>(on_success),
                             on_error = std::forward<OnError>(on_error)]() mutable {
                    struct InFlightGuard
                    {
                        explicit InFlightGuard(NanoPipeLine &line) : owner(line) {}
                        ~InFlightGuard()
                        {
                            owner.on_task_finished();
                        }
                        NanoPipeLine &owner;
                    } guard(*owner);

                    using Out = std::remove_cvref_t<decltype(std::apply(
                        [this, &cancel_token](auto &...args)
                        { return access_pipe().run_with_cancel(cancel_token, forward_captured(args)...); },
                        captured))>;

                    std::exception_ptr error;
                    std::optional<Out> output;

                    {
                        StageParallelGuard parallel_guard(*this);
                        GlobalQuotaGuard quota_guard(*owner);
                        try
                        {
                            output.emplace(std::apply(
                                [this, &cancel_token](auto &...args)
                                { return access_pipe().run_with_cancel(cancel_token, forward_captured(args)...); },
                                captured));
                        }
                        catch (...)
                        {
                            error = std::current_exception();
                        }
                    }

                    auto action = make_forward_action(
                        [on_success = std::move(on_success),
                         on_error = std::move(on_error),
                         error = std::move(error),
                         output = std::move(output)]() mutable {
                            if (error)
                            {
                                safe_call_error(on_error, error);
                                return;
                            }

                            try
                            {
                                on_success(std::move(*output));
                            }
                            catch (...)
                            {
                                safe_call_error(on_error, std::current_exception());
                            }
                        });

                    try
                    {
                        forward_in_order(seq, std::move(action));
                    }
                    catch (...)
                    {
                        safe_call_error(on_error, std::current_exception());
                    }
                };

                try
                {
                    dispatch_async(std::move(task));
                }
                catch (...)
                {
                    owner->on_task_finished();
                    safe_call_error(on_error, std::current_exception());
                }
            }

            // 同步封装（协作式取消版本）。
            template <typename... In>
            auto process(nanoai_u64 seq, const NanoCancelToken &cancel_token, In &&...input)
            {
                using Out = std::remove_cvref_t<decltype(access_pipe().run_with_cancel(cancel_token, std::forward<In>(input)...))>;

                std::binary_semaphore done_sem{0};
                std::optional<Out> output;
                std::exception_ptr error;

                process_async(
                    seq,
                    cancel_token,
                    [&](Out &&value) {
                        output.emplace(std::move(value));
                        done_sem.release();
                    },
                    [&](std::exception_ptr e) {
                        error = e;
                        done_sem.release();
                    },
                    std::forward<In>(input)...);

                done_sem.acquire();

                if (error)
                {
                    std::rethrow_exception(error);
                }
                return std::move(*output);
            }

            // Stage 关联的 pipeline 所有者。
            NanoPipeLine *owner;
            // Stage 对应 pipe（可能是值，也可能是 reference_wrapper）。
            PipeT pipe;
            // Stage 级最大并行度。
            const nanoai_u32 max_parallel;
            // 当前 stage 执行策略。
            const PipeExecutionPolicy policy;
            // dedicated 策略下的专用线程池。
            std::unique_ptr<NanoThreadPool> dedicated_pool;

            // Stage 并行许可信号量。
            std::counting_semaphore<NanoPipeLine::kMaxSemaphorePermits> parallel_sem;

            // ordered 模式的转发序状态。
            alignas(64) std::mutex order_mtx;
            alignas(64) nanoai_u64 next_forward_seq{0};

        private:
            // RAII: 获取/释放 stage 并行许可。
            struct StageParallelGuard
            {
                explicit StageParallelGuard(StageState &s) : stage(s)
                {
                    stage.parallel_sem.acquire();
                }

                ~StageParallelGuard()
                {
                    stage.parallel_sem.release();
                }

                StageState &stage;
            };

            // RAII: 获取/释放 pipeline 全局任务配额许可。
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

            // 转发动作的抽象接口，允许将任意回调对象放入同一等待队列。
            struct ForwardAction
            {
                virtual ~ForwardAction() = default;
                virtual void run() = 0;
            };

            template <typename Fn>
            struct ForwardActionImpl final : ForwardAction
            {
                explicit ForwardActionImpl(Fn &&f) : fn(std::forward<Fn>(f)) {}
                void run() override
                {
                    fn();
                }
                Fn fn;
            };

            // 与 PMR 内存池配套的删除器：负责显式析构 + 归还内存。
            struct ForwardActionDeleter
            {
                StageState *stage{nullptr};
                std::size_t bytes{0};
                std::size_t alignment{alignof(std::max_align_t)};
                void (*destroy_fn)(ForwardAction *) noexcept{nullptr};

                void operator()(ForwardAction *ptr) const noexcept
                {
                    if (ptr == nullptr)
                    {
                        return;
                    }
                    destroy_fn(ptr);
                    stage->action_resource.deallocate(ptr, bytes, alignment);
                }
            };

            using ForwardActionPtr = std::unique_ptr<ForwardAction, ForwardActionDeleter>;

            // Ordered drain 参数快照（构造后固定）。
            const nanoai_u32 drain_batch_min;
            const nanoai_u32 drain_batch_max;
            const std::chrono::microseconds drain_fast_threshold;
            const std::chrono::microseconds drain_slow_threshold;
            const bool yield_on_full_batch;
            const bool yield_on_slow_batch;

            // 当前动态批次目标，依据最近执行耗时自适应调整。
            std::atomic<nanoai_u32> drain_batch_target_;
            // action 对象内存池（线程安全，可能由多 worker 访问）。
            std::pmr::synchronized_pool_resource action_resource;
            // waiters map 专属内存池（受 order_mtx 保护，使用非线程安全池以降低开销）。
            std::pmr::unsynchronized_pool_resource order_waiters_resource;
            // 有序转发等待队列：key 为 seq，value 为待执行 action。
            std::pmr::map<nanoai_u64, ForwardActionPtr> order_waiters;

            // 单参数捕获策略：lvalue 转成 reference_wrapper，rvalue 保持可移动值。
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

            // 批量捕获输入参数，保持 lvalue/rvalue 语义。
            static constexpr auto capture_inputs(auto &&...value)
            {
                return std::tuple<std::remove_cvref_t<decltype(capture_input(std::forward<decltype(value)>(value)))>...>(
                    capture_input(std::forward<decltype(value)>(value))...);
            }

            // 从捕获存储中恢复调用参数：
            // - reference_wrapper -> 原始引用
            // - 值类型 -> move 转发
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

            // 根据 stage 策略将任务分发到 inline / dedicated / shared 执行路径。
            template <typename Task>
            void dispatch_async(Task &&task)
            {
                switch (policy)
                {
                case PipeExecutionPolicy::inline_run:
                    task();
                    return;
                case PipeExecutionPolicy::dedicated_pool:
                    if (!dedicated_pool->detach_task(std::forward<Task>(task)))
                    {
                        owner->dispatch_rejected_count_.fetch_add(1, std::memory_order_acq_rel);
                        owner->emit_event("dispatch_rejected_dedicated_pool", 0);
                        throw std::runtime_error("dedicated thread pool rejected task");
                    }
                    return;
                case PipeExecutionPolicy::shared_pool:
                default:
                    if (!owner->shared_pool_.detach_task(std::forward<Task>(task)))
                    {
                        owner->dispatch_rejected_count_.fetch_add(1, std::memory_order_acq_rel);
                        owner->emit_event("dispatch_rejected_shared_pool", 0);
                        throw std::runtime_error("shared thread pool rejected task");
                    }
                    return;
                }
            }

            template <typename OnError>
            static void safe_call_error(OnError &on_error, std::exception_ptr err) noexcept
            {
                try
                {
                    on_error(err);
                }
                catch (...)
                {
                    // 错误回调异常被吞掉，避免破坏运行时收敛路径。
                }
            }

            // 在 PMR 池上构建一个 ForwardAction 对象，减少频繁堆分配抖动。
            template <typename Fn>
            ForwardActionPtr make_forward_action(Fn &&fn)
            {
                using FnT = std::decay_t<Fn>;
                using Impl = ForwardActionImpl<FnT>;
                void *raw = action_resource.allocate(sizeof(Impl), alignof(Impl));
                auto *action = new (raw) Impl(std::forward<Fn>(fn));
                return ForwardActionPtr(
                    action,
                    ForwardActionDeleter{
                        .stage = this,
                        .bytes = sizeof(Impl),
                        .alignment = alignof(Impl),
                        .destroy_fn = [](ForwardAction *ptr) noexcept {
                            static_cast<Impl *>(ptr)->~Impl();
                        }});
            }

            // 限制 batch 大小到有效配置区间。
            nanoai_u32 clamp_drain_batch(nanoai_u32 batch) const noexcept
            {
                return std::clamp(batch, drain_batch_min, drain_batch_max);
            }

            // 基于上一批耗时与剩余积压估算下一批大小：
            // - 快且仍有积压: 增加 batch
            // - 慢: 降低 batch
            // 目标是降低尾延迟并维持吞吐稳定。
            nanoai_u32 adapt_drain_batch(
                nanoai_u32 current,
                nanoai_usize executed,
                nanoai_usize remaining,
                std::chrono::microseconds elapsed) const noexcept
            {
                nanoai_u32 next = current;
                if (elapsed <= drain_fast_threshold)
                {
                    if (remaining > executed)
                    {
                        next = current + std::max<nanoai_u32>(8, current / 4);
                    }
                }
                else if (elapsed >= drain_slow_threshold)
                {
                    next = std::max<nanoai_u32>(drain_batch_min, current / 2);
                }
                return clamp_drain_batch(next);
            }

            // 有序转发核心逻辑：
            // - unordered: 直接执行 action。
            // - ordered: 仅当 seq 命中 next_forward_seq 才能推进；否则先入 waiters。
            // - 命中后尝试连续收割后继 seq，分批执行并按耗时自适应 batch。
            void forward_in_order(nanoai_u64 seq, ForwardActionPtr action)
            {
                if constexpr (FixedOrder == PipeForwardOrder::unordered)
                {
                    action->run();
                    return;
                }

                bool run_direct{false};
                std::vector<ForwardActionPtr> ready_actions;
                {
                    std::lock_guard lock(order_mtx);
                    if (seq != next_forward_seq)
                    {
                        if (owner->options_.ordered_waiters_limit != 0 &&
                            order_waiters.size() >= owner->options_.ordered_waiters_limit)
                        {
                            owner->ordered_waiter_overflow_count_.fetch_add(1, std::memory_order_acq_rel);
                            owner->emit_event("ordered_waiters_overflow", seq);
                            throw std::runtime_error("ordered waiters overflow");
                        }
                        order_waiters.try_emplace(seq, std::move(action));
                        return;
                    }

                    // Fast path: no backlog, run current action directly without batch bookkeeping.
                    if (order_waiters.empty())
                    {
                        ++next_forward_seq;
                        run_direct = true;
                    }
                    else
                    {
                        ready_actions.reserve(order_waiters.size() + 1);
                        ready_actions.push_back(std::move(action));
                        ++next_forward_seq;

                        while (true)
                        {
                            auto next_it = order_waiters.find(next_forward_seq);
                            if (next_it == order_waiters.end())
                            {
                                break;
                            }
                            ready_actions.push_back(std::move(next_it->second));
                            order_waiters.erase(next_it);
                            ++next_forward_seq;
                        }
                    }
                }

                // 无积压时直接执行当前 action，避免额外容器开销。
                if (run_direct)
                {
                    action->run();
                    return;
                }

                // 小积压路径：直接清空，避免自适应策略本身的计时成本。
                if (ready_actions.size() <= static_cast<nanoai_usize>(drain_batch_min))
                {
                    for (auto &ready : ready_actions)
                    {
                        ready->run();
                    }
                    return;
                }

                nanoai_u32 batch_target = clamp_drain_batch(drain_batch_target_.load(std::memory_order_relaxed));
                nanoai_usize offset = 0;
                nanoai_usize total_dispatched = 0;
                while (offset < ready_actions.size())
                {
                    // 按当前 batch_target 分块执行，并用本批耗时反向调参。
                    const nanoai_u32 batch = clamp_drain_batch(batch_target);
                    nanoai_usize chunk = std::min<nanoai_usize>(batch, ready_actions.size() - offset);
                    const bool budget_enabled = owner->options_.ordered_dispatch_budget != 0;
                    if (budget_enabled)
                    {
                        const nanoai_usize consumed_in_slice = total_dispatched % owner->options_.ordered_dispatch_budget;
                        const nanoai_usize remain_budget = owner->options_.ordered_dispatch_budget - consumed_in_slice;
                        chunk = std::min(chunk, remain_budget);
                    }

                    const auto begin = std::chrono::steady_clock::now();
                    for (nanoai_usize i = 0; i < chunk; ++i)
                    {
                        ready_actions[offset + i]->run();
                    }
                    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::steady_clock::now() - begin);

                    offset += chunk;
                    total_dispatched += chunk;
                    const nanoai_usize remaining = ready_actions.size() - offset;

                    batch_target = adapt_drain_batch(batch, chunk, remaining, elapsed);
                    drain_batch_target_.store(batch_target, std::memory_order_relaxed);

                    const bool slow = yield_on_slow_batch && elapsed >= drain_slow_threshold;
                    const bool full = yield_on_full_batch && chunk == batch;
                    const bool budget_slice_end = budget_enabled && (total_dispatched % owner->options_.ordered_dispatch_budget == 0);
                    if (remaining > 0 && (slow || full || budget_slice_end))
                    {
                        // 在“慢批次”或“满批次”后主动让出时间片，避免长期占用 CPU。
                        std::this_thread::yield();
                    }
                }
            }

            // dedicated_pool 大小解析：优先使用 pipe 显式配置，否则退化为并行度。
            nanoai_u32 resolve_dedicated_pool_size() const noexcept
            {
                const nanoai_u32 configured = access_pipe().dedicated_pool_size();
                if (configured != 0)
                {
                    return configured;
                }
                return std::max<nanoai_u32>(1, access_pipe().concurrency());
            }

            // 支持 pipe 既可按值存储，也可通过 std::reference_wrapper 引用外部对象。
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

            constexpr decltype(auto) access_pipe() noexcept
            {
                return unwrap_pipe(pipe);
            }

            constexpr decltype(auto) access_pipe() const noexcept
            {
                return unwrap_pipe(pipe);
            }
        };

        // 根据 pipe 参数包创建 stage 状态对象。
        template <typename... Args>
        static auto build_stages(NanoPipeLine *owner, Args &&...args)
        {
            return std::make_tuple(std::make_shared<StageState<Pipes>>(owner, std::forward<Args>(args))...);
        }

        template <typename Tuple, nanoai_usize... I>
        static auto build_stages_from_tuple_impl(NanoPipeLine *owner, Tuple &pipes, std::index_sequence<I...>)
        {
            return build_stages(owner, std::get<I>(pipes)...);
        }

        template <typename Tuple>
        static auto build_stages_from_tuple(NanoPipeLine *owner, Tuple &pipes)
        {
            return build_stages_from_tuple_impl(owner, pipes, std::index_sequence_for<Pipes...>{});
        }

        // 统一末端返回值语义：
        // - 单返回值直接返回该值
        // - 多返回值打包为 tuple
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

        // 同步链式执行（递归模板展开）。
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

        // 异步链式执行（递归模板展开）。
        template <nanoai_usize I, typename OnSuccess, typename OnError, typename... Args>
        void run_chain_async(nanoai_u64 seq, OnSuccess &&on_success, OnError &&on_error, Args &&...args)
        {
            if constexpr (I == sizeof...(Pipes))
            {
                on_success(pack_multi_return(std::forward<Args>(args)...));
            }
            else
            {
                auto stage = std::get<I>(stages_);
                stage->process_async(
                    seq,
                    [this, seq, on_success = std::forward<OnSuccess>(on_success), on_error = std::forward<OnError>(on_error)](auto &&out) mutable {
                        forward_to_next_async<I + 1>(
                            seq,
                            std::move(on_success),
                            std::move(on_error),
                            std::forward<decltype(out)>(out));
                    },
                    [on_error = std::forward<OnError>(on_error)](std::exception_ptr e) mutable {
                        on_error(e);
                    },
                    std::forward<Args>(args)...);
            }
        }

        // 同步链式执行（协作式取消版本）。
        template <nanoai_usize I, typename... Args>
        auto run_chain_with_cancel(nanoai_u64 seq, const NanoCancelToken &cancel_token, Args &&...args)
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
                auto out = std::get<I>(stages_)->process(seq, cancel_token, std::forward<Args>(args)...);
                return forward_to_next_with_cancel<I + 1>(seq, cancel_token, std::move(out));
            }
        }

        // 异步链式执行（协作式取消版本）。
        template <nanoai_usize I, typename OnSuccess, typename OnError, typename... Args>
        void run_chain_async_with_cancel(
            nanoai_u64 seq,
            const NanoCancelToken &cancel_token,
            OnSuccess &&on_success,
            OnError &&on_error,
            Args &&...args)
        {
            if constexpr (I == sizeof...(Pipes))
            {
                on_success(pack_multi_return(std::forward<Args>(args)...));
            }
            else
            {
                auto stage = std::get<I>(stages_);
                stage->process_async(
                    seq,
                    cancel_token,
                    [this, seq, &cancel_token, on_success = std::forward<OnSuccess>(on_success), on_error = std::forward<OnError>(on_error)](auto &&out) mutable {
                        forward_to_next_async_with_cancel<I + 1>(
                            seq,
                            cancel_token,
                            std::move(on_success),
                            std::move(on_error),
                            std::forward<decltype(out)>(out));
                    },
                    [on_error = std::forward<OnError>(on_error)](std::exception_ptr e) mutable {
                        on_error(e);
                    },
                    std::forward<Args>(args)...);
            }
        }

        // 将 stage 输出转发到下一 stage：若为 tuple-like 则展开参数。
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

        // 异步转发版本，对 tuple-like 输出做展开后继续链式调度。
        template <nanoai_usize I, typename OnSuccess, typename OnError, typename Out>
        void forward_to_next_async(nanoai_u64 seq, OnSuccess &&on_success, OnError &&on_error, Out &&out)
        {
            if constexpr (TupleLike<Out>)
            {
                std::apply(
                    [this, seq, on_success = std::forward<OnSuccess>(on_success), on_error = std::forward<OnError>(on_error)](auto &&...items) mutable {
                        run_chain_async<I>(
                            seq,
                            std::move(on_success),
                            std::move(on_error),
                            std::forward<decltype(items)>(items)...);
                    },
                    std::forward<Out>(out));
            }
            else
            {
                run_chain_async<I>(
                    seq,
                    std::forward<OnSuccess>(on_success),
                    std::forward<OnError>(on_error),
                    std::forward<Out>(out));
            }
        }

        // 协作式取消版本的输出转发。
        template <nanoai_usize I, typename Out>
        auto forward_to_next_with_cancel(nanoai_u64 seq, const NanoCancelToken &cancel_token, Out &&out)
        {
            if constexpr (TupleLike<Out>)
            {
                return std::apply(
                    [this, seq, &cancel_token](auto &&...items)
                    {
                        return run_chain_with_cancel<I>(seq, cancel_token, std::forward<decltype(items)>(items)...);
                    },
                    std::forward<Out>(out));
            }
            else
            {
                return run_chain_with_cancel<I>(seq, cancel_token, std::forward<Out>(out));
            }
        }

        // 协作式取消版本的异步输出转发。
        template <nanoai_usize I, typename OnSuccess, typename OnError, typename Out>
        void forward_to_next_async_with_cancel(
            nanoai_u64 seq,
            const NanoCancelToken &cancel_token,
            OnSuccess &&on_success,
            OnError &&on_error,
            Out &&out)
        {
            if constexpr (TupleLike<Out>)
            {
                std::apply(
                    [this, seq, &cancel_token, on_success = std::forward<OnSuccess>(on_success), on_error = std::forward<OnError>(on_error)](auto &&...items) mutable {
                        run_chain_async_with_cancel<I>(
                            seq,
                            cancel_token,
                            std::move(on_success),
                            std::move(on_error),
                            std::forward<decltype(items)>(items)...);
                    },
                    std::forward<Out>(out));
            }
            else
            {
                run_chain_async_with_cancel<I>(
                    seq,
                    cancel_token,
                    std::forward<OnSuccess>(on_success),
                    std::forward<OnError>(on_error),
                    std::forward<Out>(out));
            }
        }

        // 获取全局任务配额令牌。
        void acquire_global_task_token()
        {
            global_quota_sem_.acquire();
        }

        // 归还全局任务配额令牌。
        void release_global_task_token() noexcept
        {
            global_quota_sem_.release();
        }

        bool on_task_submitted_if_accepting() noexcept
        {
            std::lock_guard<std::mutex> lock(shutdown_mtx_);
            if (!accepting_tasks_)
            {
                return false;
            }
            ++in_flight_tasks_;
            return true;
        }

        void on_task_finished() noexcept
        {
            std::lock_guard<std::mutex> lock(shutdown_mtx_);
            if (in_flight_tasks_ > 0)
            {
                --in_flight_tasks_;
            }
            if (in_flight_tasks_ == 0)
            {
                shutdown_cv_.notify_all();
            }
        }

        // 对用户 options 做边界修正，保证运行时参数合法。
        static NanoPipeLineOptions normalize_options(NanoPipeLineOptions options) noexcept
        {
            options.shared_pool_size = std::max<nanoai_u32>(1, resolve_pool_size(options.shared_pool_size));
            options.global_task_quota = std::max<nanoai_u32>(1, resolve_quota(options.global_task_quota));
            options.drain_batch_min = std::max<nanoai_u32>(1, options.drain_batch_min);
            options.drain_batch_max = std::max(options.drain_batch_min, options.drain_batch_max);
            options.drain_fast_threshold_us = std::max<nanoai_u32>(1, options.drain_fast_threshold_us);
            options.drain_slow_threshold_us = std::max(options.drain_fast_threshold_us, options.drain_slow_threshold_us);
            options.shared_pool_submit_timeout_ms = std::max<nanoai_u32>(1, options.shared_pool_submit_timeout_ms);
            options.dedicated_pool_submit_timeout_ms = std::max<nanoai_u32>(1, options.dedicated_pool_submit_timeout_ms);
            return options;
        }

        // 共享池大小解析：0 使用硬件并发，硬件并发不可用时使用 4。
        static nanoai_u32 resolve_pool_size(nanoai_u32 requested) noexcept
        {
            if (requested != 0)
            {
                return requested;
            }
            const auto hw = std::thread::hardware_concurrency();
            return std::max<nanoai_u32>(1, static_cast<nanoai_u32>(hw == 0 ? 4 : hw));
        }

        // 全局配额解析：默认取约 2x 硬件并发，以平衡吞吐与内存占用。
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

        // 归一化后的全局配置。
        NanoPipeLineOptions options_{};
        // 共享池线程数量。
        nanoai_u32 shared_pool_size_;
        // 共享线程池实例。
        NanoThreadPool shared_pool_;

        // 全局任务配额与对应令牌信号量。
        nanoai_u32 global_task_quota_;
        std::counting_semaphore<kMaxSemaphorePermits> global_quota_sem_;

        // 全局协作式取消信号。
        std::shared_ptr<NanoCancelToken> global_cancel_token_{std::make_shared<NanoCancelToken>()};

        // 运行序号生成器（用于 ordered 模式稳定排序）。
        std::atomic<nanoai_u64> next_seq_{0};
        // 用户 pipe 存储。
        std::tuple<Pipes...> pipes_;
        // 每个 pipe 对应的 stage 运行时状态。
        std::tuple<std::shared_ptr<StageState<Pipes>>...> stages_;

        // 生命周期与关闭控制。
        bool accepting_tasks_{true};
        nanoai_u64 in_flight_tasks_{0};
        mutable std::mutex shutdown_mtx_;
        std::condition_variable shutdown_cv_;

        // 运行可观测统计。
        std::atomic<nanoai_u64> run_submitted_count_{0};
        std::atomic<nanoai_u64> run_completed_count_{0};
        std::atomic<nanoai_u64> run_rejected_count_{0};
        std::atomic<nanoai_u64> run_timeout_count_{0};
        std::atomic<nanoai_u64> dispatch_rejected_count_{0};
        std::atomic<nanoai_u64> ordered_waiter_overflow_count_{0};
    };

    // 构建器：仅允许右值链式追加，避免无意的昂贵复制。
    template <PipeForwardOrder FixedOrder, typename... Pipes>
    class NanoPipeLineBuilder
    {
    public:
        // options 是构建参数快照，pipes 是已收集的 stage 列表。
        explicit NanoPipeLineBuilder(NanoPipeLineOptions options, Pipes... pipes)
            : options_(options),
              pipes_(std::move(pipes)...)
        {
        }

        template <typename P>
            requires PipeStage<std::remove_cvref_t<P>>
        auto add_pipe(P &&pipe) const & = delete;

        template <typename P>
            requires PipeStage<std::remove_cvref_t<P>>
        auto add_pipe(P &&pipe) &&
        {
            // 仅右值可追加，强制使用 move-builder 风格。
            return std::move(*this).append_move(std::forward<P>(pipe), std::index_sequence_for<Pipes...>{});
        }

        auto build() const & = delete;

        auto build() &&
        {
            // 仅右值可 build，避免从可复用 builder 反复构建造成语义歧义。
            return std::move(*this).build_move(std::index_sequence_for<Pipes...>{});
        }

    private:
        template <typename P, nanoai_usize... I>
        auto append_move(P &&pipe, std::index_sequence<I...>)
        {
            return NanoPipeLineBuilder<FixedOrder, Pipes..., std::decay_t<P>>(
                options_,
                std::move(std::get<I>(pipes_))...,
                std::forward<P>(pipe));
        }

        template <nanoai_usize... I>
        auto build_move(std::index_sequence<I...>)
        {
            return NanoPipeLine<FixedOrder, Pipes...>(
                options_,
                std::move(std::get<I>(pipes_))...);
        }

        // 构建参数与累计 pipe 容器。
        NanoPipeLineOptions options_{};
        std::tuple<Pipes...> pipes_{};
    };

    // 便捷工厂：按值推导 pipe 类型。
    template <PipeForwardOrder FixedOrder, typename... Pipes>
        requires(PipeStage<std::remove_cvref_t<Pipes>> && ...)
    auto make_pipeline(Pipes &&...pipes)
    {
        return NanoPipeLine<FixedOrder, std::decay_t<Pipes>...>(
            std::forward<Pipes>(pipes)...);
    }

    // 便捷工厂：显式设置共享池大小与全局配额。
    template <PipeForwardOrder FixedOrder, typename... Pipes>
        requires(PipeStage<std::remove_cvref_t<Pipes>> && ...)
    auto make_pipeline(nanoai_u32 shared_pool_size, nanoai_u32 global_task_quota, Pipes &&...pipes)
    {
        return NanoPipeLine<FixedOrder, std::decay_t<Pipes>...>(
            shared_pool_size,
            global_task_quota,
            std::forward<Pipes>(pipes)...);
    }

    // 便捷工厂：使用完整 options。
    template <PipeForwardOrder FixedOrder, typename... Pipes>
        requires(PipeStage<std::remove_cvref_t<Pipes>> && ...)
    auto make_pipeline(NanoPipeLineOptions options, Pipes &&...pipes)
    {
        return NanoPipeLine<FixedOrder, std::decay_t<Pipes>...>(
            options,
            std::forward<Pipes>(pipes)...);
    }

    // 便捷构建器：默认 options。
    template <PipeForwardOrder FixedOrder>
    auto make_pipeline_builder()
    {
        return NanoPipeLineBuilder<FixedOrder>(NanoPipeLineOptions{});
    }

    // 便捷构建器：显式 shared_pool_size + global_task_quota。
    template <PipeForwardOrder FixedOrder>
    auto make_pipeline_builder(nanoai_u32 shared_pool_size, nanoai_u32 global_task_quota)
    {
        return NanoPipeLineBuilder<FixedOrder>(
            NanoPipeLineOptions{shared_pool_size, global_task_quota});
    }

    // 便捷构建器：完整 options。
    template <PipeForwardOrder FixedOrder>
    auto make_pipeline_builder(NanoPipeLineOptions options)
    {
        return NanoPipeLineBuilder<FixedOrder>(options);
    }

} // namespace NanoAI_FLOW
