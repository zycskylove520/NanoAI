// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: pipeline.hpp
// Brief: 编译期特化并发流水线实现（固定 ordered/unordered）。

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <new>
#include <optional>
#include <semaphore>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <BS_thread_pool.hpp>

#include "pipe.hpp"

namespace NanoAI_FLOW
{

    enum class PipeForwardOrder
    {
        ordered,
        unordered
    };

    struct NanoPipeLineOptions
    {
        nanoai_u32 shared_pool_size{0};
        nanoai_u32 global_task_quota{0};

        // Ordered forwarding adaptive drain controls.
        nanoai_u32 drain_batch_min{8};
        nanoai_u32 drain_batch_max{512};
        nanoai_u32 drain_fast_threshold_us{80};
        nanoai_u32 drain_slow_threshold_us{300};
        bool yield_on_full_batch{true};
        bool yield_on_slow_batch{true};
    };

    template <typename T>
    concept TupleLike = requires {
        typename std::tuple_size<std::remove_reference_t<T>>::type;
    };

    template <PipeForwardOrder FixedOrder, PipeStage... Pipes>
    class NanoPipeLine
    {
    public:
        static constexpr std::ptrdiff_t kMaxSemaphorePermits = 1024 * 1024;

        explicit NanoPipeLine(Pipes... pipes)
            : NanoPipeLine(NanoPipeLineOptions{}, std::forward<Pipes>(pipes)...)
        {
        }

        NanoPipeLine(nanoai_u32 shared_pool_size, nanoai_u32 global_task_quota, Pipes... pipes)
            : NanoPipeLine(NanoPipeLineOptions{shared_pool_size, global_task_quota}, std::forward<Pipes>(pipes)...)
        {
        }

        NanoPipeLine(NanoPipeLineOptions options, Pipes... pipes)
            : options_(normalize_options(options)),
              shared_pool_size_(resolve_pool_size(options_.shared_pool_size)),
              shared_pool_(shared_pool_size_),
              global_task_quota_(resolve_quota(options_.global_task_quota)),
              global_quota_sem_(static_cast<std::ptrdiff_t>(global_task_quota_)),
              pipes_(std::forward<Pipes>(pipes)...),
              stages_(build_stages_from_tuple(this, pipes_))
        {
        }

        ~NanoPipeLine() = default;

        static constexpr PipeForwardOrder fixed_forward_order() noexcept
        {
            return FixedOrder;
        }

        nanoai_u32 shared_pool_size() const noexcept
        {
            return shared_pool_size_;
        }

        nanoai_u32 global_task_quota() const noexcept
        {
            return global_task_quota_;
        }

        static constexpr nanoai_u32 pipe_count() noexcept
        {
            return static_cast<nanoai_u32>(sizeof...(Pipes));
        }

        template <typename... Args>
        auto run(Args &&...args)
        {
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

            done_sem.acquire();

            if (error)
            {
                std::rethrow_exception(error);
            }
            return std::move(*output);
        }

    private:
        template <typename PipeT>
        struct StageState
        {
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
                if (policy == PipeExecutionPolicy::dedicated_pool)
                {
                                    dedicated_pool = std::make_unique<BS::thread_pool<>>(resolve_dedicated_pool_size());
                }
            }

            template <typename OnSuccess, typename OnError, typename... In>
            void process_async(nanoai_u64 seq, OnSuccess &&on_success, OnError &&on_error, In &&...input)
            {
                auto captured = capture_inputs(std::forward<In>(input)...);
                auto task = [this,
                             seq,
                             captured = std::move(captured),
                             on_success = std::forward<OnSuccess>(on_success),
                             on_error = std::forward<OnError>(on_error)]() mutable {
                    using Out = std::remove_cvref_t<decltype(std::apply(
                        [this](auto &...args)
                        { return access_pipe().run(forward_captured(args)...); },
                        captured))>;

                    std::exception_ptr error;
                    std::optional<Out> output;

                    {
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

                    auto action = make_forward_action(
                        [on_success = std::move(on_success),
                         on_error = std::move(on_error),
                         error = std::move(error),
                         output = std::move(output)]() mutable {
                            if (error)
                            {
                                on_error(error);
                                return;
                            }
                            on_success(std::move(*output));
                        });

                    forward_in_order(seq, std::move(action));
                };

                dispatch_async(std::move(task));
            }

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

            NanoPipeLine *owner;
            PipeT pipe;
            const nanoai_u32 max_parallel;
            const PipeExecutionPolicy policy;
            std::unique_ptr<BS::thread_pool<>> dedicated_pool;

            std::counting_semaphore<NanoPipeLine::kMaxSemaphorePermits> parallel_sem;

            alignas(64) std::mutex order_mtx;
            alignas(64) nanoai_u64 next_forward_seq{0};

        private:
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

            const nanoai_u32 drain_batch_min;
            const nanoai_u32 drain_batch_max;
            const std::chrono::microseconds drain_fast_threshold;
            const std::chrono::microseconds drain_slow_threshold;
            const bool yield_on_full_batch;
            const bool yield_on_slow_batch;

            std::atomic<nanoai_u32> drain_batch_target_;
            std::pmr::synchronized_pool_resource action_resource;
            std::pmr::unsynchronized_pool_resource order_waiters_resource;
            std::pmr::map<nanoai_u64, ForwardActionPtr> order_waiters;

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

            static constexpr auto capture_inputs(auto &&...value)
            {
                return std::tuple<std::remove_cvref_t<decltype(capture_input(std::forward<decltype(value)>(value)))>...>(
                    capture_input(std::forward<decltype(value)>(value))...);
            }

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

            template <typename Task>
            void dispatch_async(Task &&task)
            {
                switch (policy)
                {
                case PipeExecutionPolicy::inline_run:
                    task();
                    return;
                case PipeExecutionPolicy::dedicated_pool:
                    dedicated_pool->detach_task(std::forward<Task>(task));
                    return;
                case PipeExecutionPolicy::shared_pool:
                default:
                    owner->shared_pool_.detach_task(std::forward<Task>(task));
                    return;
                }
            }

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

            nanoai_u32 clamp_drain_batch(nanoai_u32 batch) const noexcept
            {
                return std::clamp(batch, drain_batch_min, drain_batch_max);
            }

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

                if (run_direct)
                {
                    action->run();
                    return;
                }

                // Small backlog path: skip adaptive batch timing overhead.
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
                while (offset < ready_actions.size())
                {
                    const nanoai_u32 batch = clamp_drain_batch(batch_target);
                    const nanoai_usize chunk = std::min<nanoai_usize>(batch, ready_actions.size() - offset);

                    const auto begin = std::chrono::steady_clock::now();
                    for (nanoai_usize i = 0; i < chunk; ++i)
                    {
                        ready_actions[offset + i]->run();
                    }
                    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::steady_clock::now() - begin);

                    offset += chunk;
                    const nanoai_usize remaining = ready_actions.size() - offset;

                    batch_target = adapt_drain_batch(batch, chunk, remaining, elapsed);
                    drain_batch_target_.store(batch_target, std::memory_order_relaxed);

                    const bool slow = yield_on_slow_batch && elapsed >= drain_slow_threshold;
                    const bool full = yield_on_full_batch && chunk == batch;
                    if (remaining > 0 && (slow || full))
                    {
                        std::this_thread::yield();
                    }
                }
            }

            nanoai_u32 resolve_dedicated_pool_size() const noexcept
            {
                const nanoai_u32 configured = access_pipe().dedicated_pool_size();
                if (configured != 0)
                {
                    return configured;
                }
                return std::max<nanoai_u32>(1, access_pipe().concurrency());
            }

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

        void acquire_global_task_token()
        {
            global_quota_sem_.acquire();
        }

        void release_global_task_token() noexcept
        {
            global_quota_sem_.release();
        }

        static NanoPipeLineOptions normalize_options(NanoPipeLineOptions options) noexcept
        {
            options.shared_pool_size = std::max<nanoai_u32>(1, resolve_pool_size(options.shared_pool_size));
            options.global_task_quota = std::max<nanoai_u32>(1, resolve_quota(options.global_task_quota));
            options.drain_batch_min = std::max<nanoai_u32>(1, options.drain_batch_min);
            options.drain_batch_max = std::max(options.drain_batch_min, options.drain_batch_max);
            options.drain_fast_threshold_us = std::max<nanoai_u32>(1, options.drain_fast_threshold_us);
            options.drain_slow_threshold_us = std::max(options.drain_fast_threshold_us, options.drain_slow_threshold_us);
            return options;
        }

        static nanoai_u32 resolve_pool_size(nanoai_u32 requested) noexcept
        {
            if (requested != 0)
            {
                return requested;
            }
            const auto hw = std::thread::hardware_concurrency();
            return std::max<nanoai_u32>(1, static_cast<nanoai_u32>(hw == 0 ? 4 : hw));
        }

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

        NanoPipeLineOptions options_{};
        nanoai_u32 shared_pool_size_;
        BS::thread_pool<> shared_pool_;

        nanoai_u32 global_task_quota_;
        std::counting_semaphore<kMaxSemaphorePermits> global_quota_sem_;

        std::atomic<nanoai_u64> next_seq_{0};
        std::tuple<Pipes...> pipes_;
        std::tuple<std::shared_ptr<StageState<Pipes>>...> stages_;
    };

    template <PipeForwardOrder FixedOrder, typename... Pipes>
    class NanoPipeLineBuilder
    {
    public:
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
            return std::move(*this).append_move(std::forward<P>(pipe), std::index_sequence_for<Pipes...>{});
        }

        auto build() const & = delete;

        auto build() &&
        {
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

        NanoPipeLineOptions options_{};
        std::tuple<Pipes...> pipes_{};
    };

    template <PipeForwardOrder FixedOrder, typename... Pipes>
        requires(PipeStage<std::remove_cvref_t<Pipes>> && ...)
    auto make_pipeline(Pipes &&...pipes)
    {
        return NanoPipeLine<FixedOrder, std::decay_t<Pipes>...>(
            std::forward<Pipes>(pipes)...);
    }

    template <PipeForwardOrder FixedOrder, typename... Pipes>
        requires(PipeStage<std::remove_cvref_t<Pipes>> && ...)
    auto make_pipeline(nanoai_u32 shared_pool_size, nanoai_u32 global_task_quota, Pipes &&...pipes)
    {
        return NanoPipeLine<FixedOrder, std::decay_t<Pipes>...>(
            shared_pool_size,
            global_task_quota,
            std::forward<Pipes>(pipes)...);
    }

    template <PipeForwardOrder FixedOrder, typename... Pipes>
        requires(PipeStage<std::remove_cvref_t<Pipes>> && ...)
    auto make_pipeline(NanoPipeLineOptions options, Pipes &&...pipes)
    {
        return NanoPipeLine<FixedOrder, std::decay_t<Pipes>...>(
            options,
            std::forward<Pipes>(pipes)...);
    }

    template <PipeForwardOrder FixedOrder>
    auto make_pipeline_builder()
    {
        return NanoPipeLineBuilder<FixedOrder>(NanoPipeLineOptions{});
    }

    template <PipeForwardOrder FixedOrder>
    auto make_pipeline_builder(nanoai_u32 shared_pool_size, nanoai_u32 global_task_quota)
    {
        return NanoPipeLineBuilder<FixedOrder>(
            NanoPipeLineOptions{shared_pool_size, global_task_quota});
    }

    template <PipeForwardOrder FixedOrder>
    auto make_pipeline_builder(NanoPipeLineOptions options)
    {
        return NanoPipeLineBuilder<FixedOrder>(options);
    }

} // namespace NanoAI_FLOW
