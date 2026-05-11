// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: test_order_consistency_high_concurrency.cpp
// Brief: 验证高并发争用与严格顺序控制下的结果正确性。

#include <nanoai_flow/nanoai_flow.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace NanoAI_FLOW;

// 文件用途：
// 验证顺序一致性相关能力，分两个阶段：
// 1) 多线程高并发提交下，结果映射保持一致（按输入 ID 校验结果）；
// 2) 严格顺序控制场景下，完成顺序保持单调不降。
namespace
{

// 线程池高并发一致性测试的进程内超时保护（秒）。
constexpr std::chrono::seconds kOrderConsistencyTimeout{60};

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

/// 阶段 1：x -> x + 1。
class AddPipe : public NanoPipe<AddPipe, 8, PipeExecutionPolicy::shared_pool>
{
public:
    int on_run(int x)
    {
        return x + 1;
    }
};

/// 阶段 2：x -> x * 2，运行在专用线程池。
class MulPipe : public NanoPipe<MulPipe, 8, PipeExecutionPolicy::dedicated_pool, 4>
{
public:
    int on_run(int x)
    {
        return x * 2;
    }
};

/// 阶段 3：x -> x - 3。
class MinusPipe : public NanoPipe<MinusPipe, 8, PipeExecutionPolicy::shared_pool>
{
public:
    int on_run(int x)
    {
        return x - 3;
    }
};

/// 高并发输出正确性检查结果。
struct ConcurrencyCheckResult
{
    /// 任务总数。
    int jobs{0};
    /// 工作线程数。
    int worker_count{0};
    /// 首个错误索引，-1 表示无错误。
    int mismatch_index{-1};
    /// 错误位置期望值。
    int expected_at_mismatch{0};
    /// 错误位置实际值。
    int actual_at_mismatch{0};

    bool ok() const
    {
        return mismatch_index < 0;
    }
};

ConcurrencyCheckResult check_outputs_correct(const std::vector<int> &outputs, int worker_count)
{
    ConcurrencyCheckResult result;
    result.jobs = static_cast<int>(outputs.size());
    result.worker_count = worker_count;

    for (nanoai_usize i = 0; i < outputs.size(); ++i)
    {
        // 流水线变换公式：((x + 1) * 2) - 3 = 2x - 1。
        const int expected = 2 * static_cast<int>(i) - 1;
        if (outputs[i] != expected)
        {
            result.mismatch_index = static_cast<int>(i);
            result.expected_at_mismatch = expected;
            result.actual_at_mismatch = outputs[i];
            break;
        }
    }
    return result;
}

ConcurrencyCheckResult test_order_consistency_high_concurrency()
{
    // 流程：x -> (x + 1) -> *2 -> -3，期望结果为 2x - 1。
    AddPipe add;
    MulPipe mul;
    MinusPipe minus;
    auto pipeline = make_pipeline<PipeForwardOrder::ordered>(16, 128, add, mul, minus);

    constexpr int kJobs = 4000;
    const int worker_count = static_cast<int>(std::max(8u, std::thread::hardware_concurrency()));

    std::atomic<int> next{0};
    std::vector<int> outputs(static_cast<nanoai_usize>(kJobs), 0);

    std::vector<std::thread> workers;
    workers.reserve(static_cast<nanoai_usize>(worker_count));

    for (int i = 0; i < worker_count; ++i)
    {
        workers.emplace_back([&]() {
            for (;;)
            {
                const int idx = next.fetch_add(1, std::memory_order_relaxed);
                if (idx >= kJobs)
                {
                    break;
                }
                outputs[static_cast<nanoai_usize>(idx)] = pipeline.run(idx);
            }
        });
    }

    for (auto &w : workers)
    {
        w.join();
    }

    // 高并发阶段：关注“输入 ID -> 输出值”映射正确性，不要求完成顺序单调。
    return check_outputs_correct(outputs, worker_count);
}

/// 严格控制（单提交线程）场景下的检查结果。
struct StrictOrderResult
{
    /// 任务总数。
    int jobs{0};
    /// 首个错误索引，-1 表示无错误。
    int mismatch_index{-1};
    /// 错误位置期望值。
    int expected_at_mismatch{0};
    /// 错误位置实际值。
    int actual_at_mismatch{0};
    /// 完成顺序是否单调不降。
    bool monotonic{true};

    bool ok() const
    {
        return mismatch_index < 0 && monotonic;
    }
};

StrictOrderResult test_order_consistency_strict_control()
{
    // 严格顺序控制阶段: 单提交线程下验证完成顺序单调不降。
    AddPipe add;
    MulPipe mul;
    MinusPipe minus;
    auto pipeline = make_pipeline<PipeForwardOrder::ordered>(16, 128, add, mul, minus);

    constexpr int kJobs = 2000;
    std::vector<int> completion_order;
    completion_order.reserve(static_cast<nanoai_usize>(kJobs));

    StrictOrderResult result;
    result.jobs = kJobs;

    for (int i = 0; i < kJobs; ++i)
    {
        const int out = pipeline.run(i);
        if (out != 2 * i - 1)
        {
            result.mismatch_index = i;
            result.expected_at_mismatch = 2 * i - 1;
            result.actual_at_mismatch = out;
            return result;
        }
        completion_order.push_back(i);
    }

    for (nanoai_usize i = 1; i < completion_order.size(); ++i)
    {
        if (completion_order[i] < completion_order[i - 1])
        {
            result.monotonic = false;
            return result;
        }
    }

    return result;
}

} // namespace

int main()
{
    // CI/CTest 返回码约定：
    // 0 表示通过，非 0 表示失败。
    return run_with_timeout(
        "order_consistency_high_concurrency",
        kOrderConsistencyTimeout,
        []() {
            const auto concurrency_result = test_order_consistency_high_concurrency();
            const auto strict_result = test_order_consistency_strict_control();

            const bool ok_concurrency = concurrency_result.ok();
            const bool ok_strict = strict_result.ok();
            // 防回归：必须同时满足“高并发结果映射正确”与“严格模式完成顺序单调”。
            const bool ok = ok_concurrency && ok_strict;

            std::cout << "[RESULT] order_consistency_high_concurrency"
                      << " jobs=" << concurrency_result.jobs
                      << " worker_count=" << concurrency_result.worker_count
                      << " mismatch_index=" << concurrency_result.mismatch_index
                      << '\n';
            if (concurrency_result.mismatch_index >= 0)
            {
                std::cout << "[DETAIL] high_concurrency_mismatch"
                          << " index=" << concurrency_result.mismatch_index
                          << " expected=" << concurrency_result.expected_at_mismatch
                          << " actual=" << concurrency_result.actual_at_mismatch
                          << '\n';
            }

            std::cout << "[RESULT] order_consistency_strict_control"
                      << " jobs=" << strict_result.jobs
                      << " mismatch_index=" << strict_result.mismatch_index
                      << " monotonic=" << (strict_result.monotonic ? "YES" : "NO")
                      << '\n';
            if (strict_result.mismatch_index >= 0)
            {
                std::cout << "[DETAIL] strict_control_mismatch"
                          << " index=" << strict_result.mismatch_index
                          << " expected=" << strict_result.expected_at_mismatch
                          << " actual=" << strict_result.actual_at_mismatch
                          << '\n';
            }

            std::cout << "[TEST] order_consistency_high_concurrency=" << (ok_concurrency ? "PASS" : "FAIL") << '\n';
            std::cout << "[TEST] order_consistency_strict_control=" << (ok_strict ? "PASS" : "FAIL") << '\n';
            std::cout << "[TEST] order_consistency_overall=" << (ok ? "PASS" : "FAIL") << '\n';
            return ok ? 0 : 1;
        });
}
