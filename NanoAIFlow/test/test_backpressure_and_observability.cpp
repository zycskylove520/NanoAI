// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: test_backpressure_and_observability.cpp
// Brief: 验证背压策略与可观测统计/事件回调。

#include <nanoai_flow/nanoai_flow.hpp>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace NanoAI_FLOW;

namespace
{

constexpr std::chrono::seconds kTestTimeout{60};

template <typename Fn>
int run_with_timeout(const char *case_name, std::chrono::seconds timeout, Fn &&fn)
{
    std::atomic<bool> done{false};
    int exit_code = 1;

    std::thread worker([&]() {
        exit_code = fn();
        done.store(true, std::memory_order_release);
    });

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!done.load(std::memory_order_acquire))
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            std::cout << "[TIMEOUT] " << case_name
                      << " exceeded " << timeout.count() << " seconds"
                      << " (possible deadlock or infinite wait)"
                      << '\n';
            worker.detach();
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    worker.join();
    return exit_code;
}

class SlowPipe final : public NanoPipe<SlowPipe, 16, PipeExecutionPolicy::shared_pool>
{
public:
    int on_run(int x)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return x + 1;
    }
};

class SlowDedicatedPipe final : public NanoPipe<SlowDedicatedPipe, 16, PipeExecutionPolicy::dedicated_pool, 2>
{
public:
    int on_run(int x)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        return x + 1;
    }
};

class SkewedDelayPipe final : public NanoPipe<SkewedDelayPipe, 16, PipeExecutionPolicy::dedicated_pool, 4>
{
public:
    int on_run(int x)
    {
        // 低序号任务显著变慢，高序号任务快速完成，制造有序等待队列积压。
        if (x < 16)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(120));
        }
        return x;
    }
};

class PassPipe final : public NanoPipe<PassPipe, 16, PipeExecutionPolicy::shared_pool>
{
public:
    int on_run(int x)
    {
        return x;
    }
};

bool test_reject_backpressure_and_stats()
{
    NanoPipeLineOptions options{};
    options.shared_pool_size = 4;
    options.global_task_quota = 64;
    options.shared_pool_queue_capacity = 8;
    options.shared_pool_submit_policy = ThreadPoolSubmitPolicy::reject;
    options.run_timeout_ms = 2000;

    std::atomic<int> event_reject{0};
    options.event_callback = [&](const char *event_name, nanoai_u64) {
        if (std::strcmp(event_name, "dispatch_rejected_shared_pool") == 0)
        {
            event_reject.fetch_add(1, std::memory_order_relaxed);
        }
    };

    SlowPipe p1;
    PassPipe p2;
    auto pipeline = make_pipeline<PipeForwardOrder::unordered>(options, p1, p2);

    constexpr int jobs = 2000;
    const int submit_threads = static_cast<int>(std::max(8u, std::thread::hardware_concurrency()));

    std::atomic<int> next{0};
    std::atomic<int> ok_count{0};
    std::atomic<int> fail_count{0};

    std::vector<std::thread> workers;
    workers.reserve(static_cast<nanoai_usize>(submit_threads));
    for (int t = 0; t < submit_threads; ++t)
    {
        workers.emplace_back([&]() {
            for (;;)
            {
                const int idx = next.fetch_add(1, std::memory_order_relaxed);
                if (idx >= jobs)
                {
                    break;
                }

                try
                {
                    (void)pipeline.run(idx);
                    ok_count.fetch_add(1, std::memory_order_relaxed);
                }
                catch (const std::runtime_error &)
                {
                    fail_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto &w : workers)
    {
        w.join();
    }

    pipeline.shutdown();
    const auto s = pipeline.stats();

    // 防回归：出现提交失败时，统计/事件至少有一个维度能反映背压拒绝。
    const bool any_reject = (fail_count.load(std::memory_order_relaxed) > 0) &&
                            (s.dispatch_rejected > 0 || s.run_rejected > 0 || event_reject.load(std::memory_order_relaxed) > 0);

    std::cout << "[RESULT] backpressure_reject"
              << " ok_count=" << ok_count.load(std::memory_order_relaxed)
              << " fail_count=" << fail_count.load(std::memory_order_relaxed)
              << " dispatch_rejected=" << s.dispatch_rejected
              << " run_rejected=" << s.run_rejected
              << " event_reject=" << event_reject.load(std::memory_order_relaxed)
              << '\n';

    return any_reject;
}

bool test_ordered_waiters_overflow()
{
    NanoPipeLineOptions options{};
    options.shared_pool_size = 2;
    options.global_task_quota = 128;
    options.ordered_waiters_limit = 4;
    options.run_timeout_ms = 2000;

    std::atomic<int> overflow_event{0};
    options.event_callback = [&](const char *event_name, nanoai_u64) {
        if (std::strcmp(event_name, "ordered_waiters_overflow") == 0)
        {
            overflow_event.fetch_add(1, std::memory_order_relaxed);
        }
    };

    SkewedDelayPipe p1;
    PassPipe p2;
    auto pipeline = make_pipeline<PipeForwardOrder::ordered>(options, p1, p2);

    constexpr int jobs = 800;
    const int submit_threads = static_cast<int>(std::max(8u, std::thread::hardware_concurrency()));

    std::atomic<int> next{0};
    std::atomic<int> overflow_fail{0};

    std::vector<std::thread> workers;
    workers.reserve(static_cast<nanoai_usize>(submit_threads));
    for (int t = 0; t < submit_threads; ++t)
    {
        workers.emplace_back([&]() {
            for (;;)
            {
                const int idx = next.fetch_add(1, std::memory_order_relaxed);
                if (idx >= jobs)
                {
                    break;
                }
                try
                {
                    (void)pipeline.run(idx);
                }
                catch (const std::runtime_error &e)
                {
                    if (std::strstr(e.what(), "ordered waiters overflow") != nullptr)
                    {
                        overflow_fail.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            }
        });
    }

    for (auto &w : workers)
    {
        w.join();
    }

    pipeline.shutdown();
    const auto s = pipeline.stats();

    std::cout << "[RESULT] ordered_waiters_overflow"
              << " overflow_fail=" << overflow_fail.load(std::memory_order_relaxed)
              << " overflow_event=" << overflow_event.load(std::memory_order_relaxed)
              << " overflow_stats=" << s.ordered_waiter_overflow
              << '\n';

    // 防回归：有序等待队列溢出需在异常、事件或统计三条观测链路中至少出现一条。
    return overflow_fail.load(std::memory_order_relaxed) > 0 ||
           overflow_event.load(std::memory_order_relaxed) > 0 ||
           s.ordered_waiter_overflow > 0;
}

} // namespace

int main()
{
    return run_with_timeout(
        "backpressure_and_observability",
        kTestTimeout,
        []() {
            const bool ok1 = test_reject_backpressure_and_stats();
            const bool ok2 = test_ordered_waiters_overflow();
            const bool ok = ok1 && ok2;
            std::cout << "[TEST] backpressure_reject=" << (ok1 ? "PASS" : "FAIL") << '\n';
            std::cout << "[TEST] ordered_waiters_overflow=" << (ok2 ? "PASS" : "FAIL") << '\n';
            std::cout << "[TEST] backpressure_and_observability=" << (ok ? "PASS" : "FAIL") << '\n';
            return ok ? 0 : 1;
        });
}
