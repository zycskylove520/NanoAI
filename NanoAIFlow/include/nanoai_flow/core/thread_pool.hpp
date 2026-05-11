// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: thread_pool.hpp
// Brief: 基于 C++20 标准并发原语的有界线程池执行器。
//
// Design notes:
// - 支持有界任务队列，提供阻塞/超时/拒绝三种提交策略。
// - 支持 move-only 任务，兼容 pipeline 中捕获 unique_ptr/一次性资源的场景。
// - 提供异常兜底回调与运行统计，便于生产环境可观测性。
// - 析构时自动停机并 join，保证不会留下后台线程。

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "types.h"

namespace NanoAI_FLOW
{

enum class ThreadPoolSubmitPolicy
{
    // 队列满时阻塞等待可用槽位。
    block,
    // 队列满时等待超时后失败。
    timeout,
    // 队列满时立即失败。
    reject
};

// 线程池构造参数。
// 取值约束：
// - thread_count == 0 时自动推导；
// - max_queue_size == 0 时按线程数推导；
// - submit_timeout_ms 仅在 timeout 策略下生效。
struct NanoThreadPoolOptions
{
    // 工作线程数量。0 表示自动按硬件并发推导。
    nanoai_u32 thread_count{0};
    // 任务队列上限。0 表示按线程数自动推导。
    nanoai_usize max_queue_size{0};
    // 提交策略：block / timeout / reject。
    ThreadPoolSubmitPolicy submit_policy{ThreadPoolSubmitPolicy::block};
    // timeout 策略下等待时长（毫秒）。
    nanoai_u32 submit_timeout_ms{1000};
};

// 线程池统计快照。
// 说明：
// - 所有计数均为“调用 stats() 时刻”的一致性近似快照；
// - queued 通过互斥锁读取，其余计数来自原子变量。
struct NanoThreadPoolStats
{
    // 被成功入队的任务数。
    nanoai_u64 submitted{0};
    // 正常执行完成的任务数。
    nanoai_u64 completed{0};
    // 提交被拒绝的任务数（含 stop/队列满策略）。
    nanoai_u64 rejected{0};
    // timeout 提交策略下等待超时次数。
    nanoai_u64 submit_timeout{0};
    // 任务执行阶段抛异常次数。
    nanoai_u64 task_exceptions{0};
    // 当前队列任务数。
    nanoai_usize queued{0};
    // 当前正在执行任务数。
    nanoai_usize running{0};
    // 当前队列容量上限。
    nanoai_usize queue_capacity{0};
    // 历史队列峰值水位。
    nanoai_usize high_watermark{0};
};

// NanoThreadPool：有界、可观测、支持 move-only 任务的线程池。
// 生命周期语义：
// - 不可拷贝/不可移动，析构时自动 shutdown + join；
// - shutdown 可重入，多次调用安全。
// 线程安全语义：
// - 任务队列由 queue_mtx_ + 条件变量保护；
// - 统计计数使用原子，异常回调用独立锁保护。
class NanoThreadPool
{
public:
    // 任务异常回调类型。回调收到异常指针，可用于日志/监控上报。
    using ExceptionHandler = std::function<void(std::exception_ptr)>;

    // 以完整 options 构建线程池。
    // 行为：构造时立即创建 worker 线程并进入等待队列状态。
    explicit NanoThreadPool(NanoThreadPoolOptions options = {})
        : options_(normalize_options(options))
    {
        const nanoai_u32 count = resolve_thread_count(options_.thread_count);
        queue_capacity_ = resolve_queue_capacity(options_.max_queue_size, count);
        workers_.reserve(static_cast<nanoai_usize>(count));
        for (nanoai_u32 i = 0; i < count; ++i)
        {
            workers_.emplace_back([this]() { worker_loop(); });
        }
    }

    // 兼容简化构造：仅指定线程数，其余参数使用默认策略。
    explicit NanoThreadPool(nanoai_u32 thread_count)
        : NanoThreadPool(NanoThreadPoolOptions{.thread_count = thread_count})
    {
    }

    NanoThreadPool(const NanoThreadPool &) = delete;
    NanoThreadPool &operator=(const NanoThreadPool &) = delete;
    NanoThreadPool(NanoThreadPool &&) = delete;
    NanoThreadPool &operator=(NanoThreadPool &&) = delete;

    ~NanoThreadPool()
    {
        shutdown();
    }

    // 提交异步任务。
    // 返回：
    // - true: 任务成功入队；
    // - false: 因 stop/背压策略未入队。
    // 并发行为：
    // - block 策略可能阻塞当前提交线程等待队列可用槽位。
    template <typename Task>
    bool detach_task(Task &&task)
    {
        auto wrapped = MoveOnlyTask::make(std::forward<Task>(task));

        std::unique_lock<std::mutex> lock(queue_mtx_);
        if (stopping_)
        {
            rejected_count_.fetch_add(1, std::memory_order_acq_rel);
            return false;
        }

        bool accepted = false;
        switch (options_.submit_policy)
        {
        case ThreadPoolSubmitPolicy::block:
            queue_not_full_cv_.wait(lock, [this] {
                return stopping_ || tasks_.size() < queue_capacity_;
            });
            accepted = !stopping_;
            break;
        case ThreadPoolSubmitPolicy::timeout:
            if (queue_not_full_cv_.wait_for(
                    lock,
                    std::chrono::milliseconds(options_.submit_timeout_ms),
                    [this] { return stopping_ || tasks_.size() < queue_capacity_; }))
            {
                accepted = !stopping_;
            }
            else
            {
                submit_timeout_count_.fetch_add(1, std::memory_order_acq_rel);
            }
            break;
        case ThreadPoolSubmitPolicy::reject:
            accepted = (!stopping_ && tasks_.size() < queue_capacity_);
            break;
        }

        if (!accepted)
        {
            rejected_count_.fetch_add(1, std::memory_order_acq_rel);
            return false;
        }

        tasks_.emplace_back(std::move(wrapped));
        const auto queued = static_cast<nanoai_usize>(tasks_.size());
        submitted_count_.fetch_add(1, std::memory_order_acq_rel);
        update_high_watermark(queued);

        lock.unlock();
        queue_not_empty_cv_.notify_one();
        return true;
    }

    // 停机并等待所有 worker 线程退出。
    // 并发行为：
    // - 将 stopping_ 置位后唤醒所有等待线程；
    // - 可重复调用，后续调用仅执行 join 检查。
    void shutdown()
    {
        std::unique_lock<std::mutex> lock(queue_mtx_);
        if (stopping_)
        {
            lock.unlock();
            join_workers();
            return;
        }

        stopping_ = true;
        lock.unlock();

        queue_not_empty_cv_.notify_all();
        queue_not_full_cv_.notify_all();
        join_workers();
    }

    // 预留的超时停机接口。
    // 当前实现等价于 shutdown() 全等待，返回 true 表示已收敛。
    [[nodiscard]] bool shutdown_for(std::chrono::milliseconds)
    {
        shutdown();
        return true;
    }

    // 获取 worker 线程数。
    [[nodiscard]] nanoai_u32 thread_count() const noexcept
    {
        return static_cast<nanoai_u32>(workers_.size());
    }

    // 设置任务异常回调。
    // 注意：回调应避免阻塞过久，以免影响 worker 吞吐。
    void set_exception_handler(ExceptionHandler handler)
    {
        std::lock_guard<std::mutex> lock(exception_handler_mtx_);
        exception_handler_ = std::move(handler);
    }

    // 查询任务异常累计计数。
    [[nodiscard]] nanoai_u64 task_exception_count() const noexcept
    {
        return task_exception_count_.load(std::memory_order_relaxed);
    }

    // 重置任务异常计数（仅影响统计，不影响异常处理逻辑）。
    void reset_task_exception_count() noexcept
    {
        task_exception_count_.store(0, std::memory_order_relaxed);
    }

    // 读取运行统计快照。
    [[nodiscard]] NanoThreadPoolStats stats() const noexcept
    {
        NanoThreadPoolStats s;
        s.submitted = submitted_count_.load(std::memory_order_relaxed);
        s.completed = completed_count_.load(std::memory_order_relaxed);
        s.rejected = rejected_count_.load(std::memory_order_relaxed);
        s.submit_timeout = submit_timeout_count_.load(std::memory_order_relaxed);
        s.task_exceptions = task_exception_count_.load(std::memory_order_relaxed);
        s.running = running_count_.load(std::memory_order_relaxed);
        s.high_watermark = queue_high_watermark_.load(std::memory_order_relaxed);
        s.queue_capacity = queue_capacity_;
        {
            std::lock_guard<std::mutex> lock(queue_mtx_);
            s.queued = static_cast<nanoai_usize>(tasks_.size());
        }
        return s;
    }

private:
    // 解析线程数：requested 为 0 时按硬件并发推导，兜底 4。
    static nanoai_u32 resolve_thread_count(nanoai_u32 requested) noexcept
    {
        if (requested != 0)
        {
            return requested;
        }

        const auto hw = std::thread::hardware_concurrency();
        return static_cast<nanoai_u32>(hw == 0 ? 4 : hw);
    }

    // 归一化 options：保证关键参数有合法下界。
    static NanoThreadPoolOptions normalize_options(NanoThreadPoolOptions options) noexcept
    {
        options.thread_count = std::max<nanoai_u32>(1, resolve_thread_count(options.thread_count));
        options.submit_timeout_ms = std::max<nanoai_u32>(1, options.submit_timeout_ms);
        return options;
    }

    // 解析队列容量：未指定时按线程数线性扩展，避免过小队列造成抖动。
    static nanoai_usize resolve_queue_capacity(nanoai_usize requested, nanoai_u32 thread_count) noexcept
    {
        if (requested != 0)
        {
            return std::max<nanoai_usize>(1, requested);
        }
        return std::max<nanoai_usize>(256, static_cast<nanoai_usize>(thread_count) * 256);
    }

    // MoveOnlyTask: 轻量 type-erasure 容器，支持 move-only 可调用对象。
    // 用于承载含 unique_ptr/一次性资源捕获的任务闭包。
    class MoveOnlyTask
    {
    public:
        MoveOnlyTask() = default;

        // 工厂函数：将任意可调用对象封装为 MoveOnlyTask。
        template <typename Fn>
        static MoveOnlyTask make(Fn &&fn)
        {
            using FnT = std::decay_t<Fn>;
            struct Impl final : Concept
            {
                explicit Impl(FnT &&f) : fn(std::move(f)) {}
                void call() override
                {
                    fn();
                }
                FnT fn;
            };

            MoveOnlyTask t;
            t.fn_ = std::make_unique<Impl>(std::forward<Fn>(fn));
            return t;
        }

        MoveOnlyTask(MoveOnlyTask &&) noexcept = default;
        MoveOnlyTask &operator=(MoveOnlyTask &&) noexcept = default;
        MoveOnlyTask(const MoveOnlyTask &) = delete;
        MoveOnlyTask &operator=(const MoveOnlyTask &) = delete;

        explicit operator bool() const noexcept
        {
            return static_cast<bool>(fn_);
        }

        void operator()()
        {
            fn_->call();
        }

    private:
        struct Concept
        {
            virtual ~Concept() = default;
            virtual void call() = 0;
        };

        std::unique_ptr<Concept> fn_;
    };

    // worker 主循环：
    // 1) 等待队列非空或 stop；
    // 2) 取任务执行；
    // 3) 更新统计并处理异常回调。
    void worker_loop()
    {
        for (;;)
        {
            MoveOnlyTask task;
            {
                std::unique_lock<std::mutex> lock(queue_mtx_);
                queue_not_empty_cv_.wait(lock, [this] {
                    return stopping_ || !tasks_.empty();
                });

                if (stopping_ && tasks_.empty())
                {
                    break;
                }

                task = std::move(tasks_.front());
                tasks_.pop_front();
            }

            queue_not_full_cv_.notify_one();

            if (task)
            {
                running_count_.fetch_add(1, std::memory_order_acq_rel);
                try
                {
                    task();
                    completed_count_.fetch_add(1, std::memory_order_acq_rel);
                }
                catch (...)
                {
                    task_exception_count_.fetch_add(1, std::memory_order_acq_rel);

                    ExceptionHandler handler;
                    {
                        std::lock_guard<std::mutex> lock(exception_handler_mtx_);
                        handler = exception_handler_;
                    }

                    if (handler)
                    {
                        try
                        {
                            handler(std::current_exception());
                        }
                        catch (...)
                        {
                        }
                    }
                }
                running_count_.fetch_sub(1, std::memory_order_acq_rel);
            }
        }
    }

    // join 全部 worker 线程。
    void join_workers()
    {
        for (auto &worker : workers_)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
    }

    // 更新队列历史峰值，使用 CAS 避免额外锁。
    void update_high_watermark(nanoai_usize queued) noexcept
    {
        nanoai_usize cur = queue_high_watermark_.load(std::memory_order_relaxed);
        while (queued > cur && !queue_high_watermark_.compare_exchange_weak(
                                   cur,
                                   queued,
                                   std::memory_order_release,
                                   std::memory_order_relaxed))
        {
        }
    }

    // 构造参数快照。
    NanoThreadPoolOptions options_{};

    // worker 线程集合。
    std::vector<std::thread> workers_;
    // 任务队列锁与队列本体。
    mutable std::mutex queue_mtx_;
    std::deque<MoveOnlyTask> tasks_;

    // 队列空/满条件变量，用于 worker 等待与 submit 背压。
    std::condition_variable queue_not_empty_cv_;
    std::condition_variable queue_not_full_cv_;
    // stop 标志：true 表示拒绝新任务并驱动 worker 退出。
    bool stopping_{false};
    // 队列容量上限。
    nanoai_usize queue_capacity_{256};

    // 统计计数器。
    std::atomic<nanoai_u64> submitted_count_{0};
    std::atomic<nanoai_u64> completed_count_{0};
    std::atomic<nanoai_u64> rejected_count_{0};
    std::atomic<nanoai_u64> submit_timeout_count_{0};
    std::atomic<nanoai_usize> running_count_{0};
    std::atomic<nanoai_usize> queue_high_watermark_{0};

    // 异常处理回调及其保护锁。
    std::mutex exception_handler_mtx_;
    ExceptionHandler exception_handler_{};
    // 任务执行异常计数。
    std::atomic<nanoai_u64> task_exception_count_{0};
};

} // namespace NanoAI_FLOW
