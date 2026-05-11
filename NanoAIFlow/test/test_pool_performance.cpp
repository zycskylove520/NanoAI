// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: test_pool_performance.cpp
// Brief: 比较不同执行策略下流水线性能与正确性。

#include <nanoai_flow/nanoai_flow.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace NanoAI_FLOW;

// 文件用途：
// 对三种线程池策略做可运行性能测试与正确性验证：
// 1) 全共享池(shared_pool)
// 2) 全独立池(dedicated_pool)
// 3) 混合池(shared + dedicated + shared)
namespace
{

// 线程池相关性能测试的进程内超时保护（秒）。
// 该保护用于“直接运行测试可执行文件”场景，与 CTest TIMEOUT 互补。
constexpr std::chrono::seconds kPoolPerfTimeout{60};

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

/// 确定性 CPU 密集核函数，便于不同策略间可比性测试。
inline int cpu_spin_mix(int x)
{
    int acc = x;
    for (int i = 0; i < 120; ++i)
    {
        acc = (acc * 131 + i) ^ (acc >> 1);
    }
    return acc;
}

template <PipeExecutionPolicy Policy, nanoai_u32 Concurrency = 8, nanoai_u32 DedicatedSize = 0>
class ComputePipe final : public NanoPipe<ComputePipe<Policy, Concurrency, DedicatedSize>, Concurrency, Policy, DedicatedSize>
{
public:
    explicit ComputePipe(int bias) noexcept : bias_(bias)
    {
    }

    int on_run(int input)
    {
        return cpu_spin_mix(input) + bias_;
    }

private:
    /// 阶段偏置，用于构造非平凡多阶段组合结果。
    int bias_;
};

/// 单个性能用例汇总结果。
struct PerfResult
{
    /// 用例名称。
    std::string name;
    /// 任务总数。
    int jobs{0};
    /// 提交线程数。
    int submit_threads{0};
    /// 总耗时（毫秒）。
    double time_ms{0.0};
    /// 吞吐（每秒请求数）。
    double qps{0.0};
    /// 输出校验和。
    nanoai_u64 checksum{0};
    /// 输出是否正确。
    bool output_ok{false};
};

template <typename PipelineFactory>
PerfResult run_perf_case(const std::string &name, int jobs, int submit_threads, PipelineFactory make_pipeline)
{
    // 为当前用例构建一条强类型流水线。
    auto pipeline = make_pipeline();

    std::vector<int> outputs(static_cast<nanoai_usize>(jobs), 0);
    std::atomic<int> next{0};
    std::atomic<nanoai_u64> checksum{0};

    const auto begin = std::chrono::steady_clock::now();

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
                const int out = pipeline.run(idx);
                outputs[static_cast<nanoai_usize>(idx)] = out;
                checksum.fetch_add(static_cast<nanoai_u64>(out), std::memory_order_relaxed);
            }
        });
    }

    for (auto &w : workers)
    {
        w.join();
    }

    const auto end = std::chrono::steady_clock::now();
    const double time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    const double qps = (time_ms > 0.0) ? (static_cast<double>(jobs) * 1000.0 / time_ms) : 0.0;

    bool output_ok = true;
    for (int i = 0; i < jobs; ++i)
    {
        // 三阶段组合公式对应的真值。
        const int expected = cpu_spin_mix(cpu_spin_mix(cpu_spin_mix(i) + 3) + 5) + 7;
        if (outputs[static_cast<nanoai_usize>(i)] != expected)
        {
            output_ok = false;
            break;
        }
    }

    PerfResult result;
    result.name = name;
    result.jobs = jobs;
    result.submit_threads = submit_threads;
    result.time_ms = time_ms;
    result.qps = qps;
    result.checksum = checksum.load(std::memory_order_relaxed);
    result.output_ok = output_ok;
    return result;
}

void print_result(const PerfResult &r)
{
    std::cout << "[PERF] " << r.name
              << " jobs=" << r.jobs
              << " submit_threads=" << r.submit_threads
              << " time_ms=" << r.time_ms
              << " qps=" << std::fixed << std::setprecision(2) << r.qps
              << " checksum=" << r.checksum
              << " output_ok=" << (r.output_ok ? "PASS" : "FAIL")
              << '\n';
}

bool test_pool_performance_and_correctness()
{
    // 不使用固定性能阈值，避免因机器差异导致误报。
    // 通过条件: 三种策略都输出正确结果。
    const int jobs = 8000;
    const int submit_threads = static_cast<int>(std::max(4u, std::thread::hardware_concurrency()));

    const auto shared = run_perf_case(
        "shared_pool_all",
        jobs,
        submit_threads,
        []() {
            auto p1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto p2 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(5);
            auto p3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline<PipeForwardOrder::ordered>(16, 64, p1, p2, p3);
        });

    const auto dedicated = run_perf_case(
        "dedicated_pool_all",
        jobs,
        submit_threads,
        []() {
            auto p1 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(3);
            auto p2 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(5);
            auto p3 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(7);
            return make_pipeline<PipeForwardOrder::ordered>(16, 64, p1, p2, p3);
        });

    const auto mixed = run_perf_case(
        "mixed_pool_shared_dedicated_shared",
        jobs,
        submit_threads,
        []() {
            auto p1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto p2 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(5);
            auto p3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline<PipeForwardOrder::ordered>(16, 64, p1, p2, p3);
        });

    print_result(shared);
    print_result(dedicated);
    print_result(mixed);

    const NanoPipeLineOptions mode_profile{16, 64};

    const auto mode_ordered = run_perf_case(
        "mode_ordered",
        jobs,
        submit_threads,
        [&]() {
            auto p1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto p2 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(5);
            auto p3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline<PipeForwardOrder::ordered>(mode_profile, p1, p2, p3);
        });

    const auto mode_unordered = run_perf_case(
        "mode_unordered",
        jobs,
        submit_threads,
        [&]() {
            auto p1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto p2 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(5);
            auto p3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline<PipeForwardOrder::unordered>(mode_profile, p1, p2, p3);
        });

    print_result(mode_ordered);
    print_result(mode_unordered);

    // 防回归：本用例只把“输出正确性”作为通过条件，不绑定机器相关的性能阈值。
    const int pass_cases = static_cast<int>(shared.output_ok) + static_cast<int>(dedicated.output_ok) + static_cast<int>(mixed.output_ok) +
                           static_cast<int>(mode_ordered.output_ok) + static_cast<int>(mode_unordered.output_ok);
    std::cout << "[RESULT] pool_performance"
              << " pass_cases=" << pass_cases
                        << "/5"
              << " jobs=" << jobs
              << " submit_threads=" << submit_threads
              << '\n';

        return shared.output_ok && dedicated.output_ok && mixed.output_ok &&
            mode_ordered.output_ok && mode_unordered.output_ok;
}

} // namespace

int main()
{
    // CI/CTest 返回码约定：
    // 0 表示通过，非 0 表示失败。
    return run_with_timeout(
        "pool_performance",
        kPoolPerfTimeout,
        []() {
            const bool ok = test_pool_performance_and_correctness();
            std::cout << "[TEST] pool_performance=" << (ok ? "PASS" : "FAIL") << '\n';
            return ok ? 0 : 1;
        });
}
