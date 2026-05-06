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

template <typename T>
concept TupleLike = requires
{
    typename std::tuple_size<std::remove_reference_t<T>>::type;
};

template <PipeStage... Pipes>
class NanoPipeLine
{
public:
    explicit NanoPipeLine(Pipes... pipes)
        : NanoPipeLine(0, 0, std::forward<Pipes>(pipes)...)
    {
    }

    NanoPipeLine(PipeCount shared_pool_size, PipeCount global_task_quota, Pipes... pipes)
        : shared_pool_size_(resolve_pool_size(shared_pool_size)),
          shared_pool_(shared_pool_size_),
          global_task_quota_(resolve_quota(global_task_quota)),
          pipes_(std::forward<Pipes>(pipes)...),
          stages_(build_stages_from_tuple(this, pipes_))
    {
    }

    ~NanoPipeLine() = default;

    PipeCount shared_pool_size() const noexcept
    {
        return shared_pool_size_;
    }

    PipeCount global_task_quota() const noexcept
    {
        return global_task_quota_;
    }

    void set_global_task_quota(PipeCount quota)
    {
        std::lock_guard lock(quota_mtx_);
        global_task_quota_ = std::max<PipeCount>(1, quota);
        quota_cv_.notify_all();
    }

    static constexpr PipeCount pipe_count() noexcept
    {
        return static_cast<PipeCount>(sizeof...(Pipes));
    }

    template <typename P>
        requires PipeStage<std::remove_cvref_t<P>>
    auto add_pipe(P &&pipe) &
    {
        return add_pipe_impl(*this, std::forward<P>(pipe));
    }

    template <typename P>
        requires PipeStage<std::remove_cvref_t<P>>
    auto add_pipe(P &&pipe) const &
    {
        return add_pipe_impl(*this, std::forward<P>(pipe));
    }

    template <typename P>
        requires PipeStage<std::remove_cvref_t<P>>
    auto add_pipe(P &&pipe) &&
    {
        return add_pipe_impl(std::move(*this), std::forward<P>(pipe));
    }

    template <typename... Args>
    auto run(Args &&...args)
    {
        const auto seq = next_seq_.fetch_add(1, std::memory_order_relaxed);
        return run_chain<0>(seq, std::forward<Args>(args)...);
    }

    template <typename Out, typename... Args>
    Out run_as(Args &&...args)
    {
        return static_cast<Out>(run(std::forward<Args>(args)...));
    }

private:
    template <typename PipeT>
    struct StageState
    {
        explicit StageState(NanoPipeLine *owner_line, PipeT p)
            : owner(owner_line),
              pipe(std::move(p)),
              max_parallel(std::max<PipeCount>(1, access_pipe().concurrency())),
              policy(access_pipe().execution_policy())
        {
            if (policy == PipeExecutionPolicy::dedicated_pool)
            {
                dedicated_pool = std::make_unique<BS::thread_pool<>>(resolve_dedicated_pool_size());
            }
        }

        template <typename... In>
        auto process(std::uint64_t seq, In &&...input)
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
                ++next_forward_seq;
            }
            order_cv.notify_all();

            if (error)
            {
                std::rethrow_exception(error);
            }
            return std::move(*output);
        }

        NanoPipeLine *owner;
        PipeT pipe;
        const PipeCount max_parallel;
        const PipeExecutionPolicy policy;
        std::unique_ptr<BS::thread_pool<>> dedicated_pool;

        std::mutex parallel_mtx;
        std::condition_variable parallel_cv;
        PipeCount active_count{0};

        std::mutex order_mtx;
        std::condition_variable order_cv;
        std::uint64_t next_forward_seq{0};

    private:
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
                                                   {
                                                       return std::apply([this](auto &...args)
                                                                         { return access_pipe().run(forward_captured(args)...); },
                                                                         captured);
                                                   })
                    .get();
            }
            case PipeExecutionPolicy::shared_pool:
            default:
            {
                auto captured = capture_inputs(std::forward<In>(input)...);
                return owner->shared_pool_.submit_task([this, captured = std::move(captured)]() mutable
                                                       {
                                                           return std::apply([this](auto &...args)
                                                                             { return access_pipe().run(forward_captured(args)...); },
                                                                             captured);
                                                       })
                    .get();
            }
            }
        }

        PipeCount resolve_dedicated_pool_size() const noexcept
        {
            const PipeCount configured = access_pipe().dedicated_pool_size();
            if (configured != 0)
            {
                return configured;
            }
            return std::max<PipeCount>(1, access_pipe().concurrency());
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

    template <typename Tuple, std::size_t... I>
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

    template <std::size_t I, typename... Args>
    auto run_chain(std::uint64_t seq, Args &&...args)
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

    template <std::size_t I, typename Out>
    auto forward_to_next(std::uint64_t seq, Out &&out)
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

    template <typename Self, typename P, std::size_t... I>
    static auto add_pipe_impl_with_indices(Self &&self, P &&pipe, std::index_sequence<I...>)
    {
        return NanoPipeLine<Pipes..., std::decay_t<P>>(
            self.shared_pool_size_,
            self.global_task_quota_,
            std::get<I>(std::forward<Self>(self).pipes_)...,
            std::forward<P>(pipe));
    }

    template <typename Self, typename P>
    static auto add_pipe_impl(Self &&self, P &&pipe)
    {
        return add_pipe_impl_with_indices(
            std::forward<Self>(self),
            std::forward<P>(pipe),
            std::index_sequence_for<Pipes...>{});
    }

    void acquire_global_task_token()
    {
        std::unique_lock lock(quota_mtx_);
        quota_cv_.wait(lock, [this]
                       { return global_tasks_in_use_ < global_task_quota_; });
        ++global_tasks_in_use_;
    }

    void release_global_task_token() noexcept
    {
        {
            std::lock_guard lock(quota_mtx_);
            --global_tasks_in_use_;
        }
        quota_cv_.notify_one();
    }

    static PipeCount resolve_pool_size(PipeCount requested) noexcept
    {
        if (requested != 0)
        {
            return requested;
        }
        const auto hw = std::thread::hardware_concurrency();
        return std::max<PipeCount>(1, static_cast<PipeCount>(hw == 0 ? 4 : hw));
    }

    static PipeCount resolve_quota(PipeCount requested) noexcept
    {
        if (requested != 0)
        {
            return requested;
        }
        const auto hw = std::thread::hardware_concurrency();
        const PipeCount base = static_cast<PipeCount>(hw == 0 ? 4 : hw);
        return std::max<PipeCount>(2, static_cast<PipeCount>(base * 2));
    }

    PipeCount shared_pool_size_;
    BS::thread_pool<> shared_pool_;

    std::mutex quota_mtx_;
    std::condition_variable quota_cv_;
    PipeCount global_task_quota_;
    PipeCount global_tasks_in_use_{0};

    std::atomic<std::uint64_t> next_seq_{0};
    std::tuple<Pipes...> pipes_;
    std::tuple<std::shared_ptr<StageState<Pipes>>...> stages_;
};

template <typename... Pipes>
NanoPipeLine(Pipes...) -> NanoPipeLine<std::decay_t<Pipes>...>;

template <typename... Pipes>
NanoPipeLine(PipeCount, PipeCount, Pipes...) -> NanoPipeLine<std::decay_t<Pipes>...>;

template <typename BuildFn>
class NanoPipeLineBuilder
{
public:
    explicit NanoPipeLineBuilder(BuildFn fn)
        : build_fn_(std::move(fn))
    {
    }

    template <typename P>
        requires PipeStage<std::remove_cvref_t<P>>
    auto add_pipe(P &&pipe) &
    {
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

    decltype(auto) build() &&
    {
        return std::move(build_fn_)();
    }

    decltype(auto) build() const &
    {
        return build_fn_();
    }

private:
    BuildFn build_fn_;
};

inline auto make_pipeline_builder(PipeCount shared_pool_size = 0, PipeCount global_task_quota = 0)
{
    return NanoPipeLineBuilder([shared_pool_size, global_task_quota]()
                               { return NanoPipeLine<>(shared_pool_size, global_task_quota); });
}

inline auto NanoPipeBuilder(PipeCount shared_pool_size = 0, PipeCount global_task_quota = 0)
{
    return make_pipeline_builder(shared_pool_size, global_task_quota);
}

template <typename Fn>
NanoPipeLineBuilder(Fn) -> NanoPipeLineBuilder<std::decay_t<Fn>>;

} // namespace NanoAI_FLOW
