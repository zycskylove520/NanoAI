// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: perf_benchmark.cpp
// Brief: 高并发性能基准，比较 shared/dedicated/mixed 三种执行策略。

#include "../include/nanoai_flow/core/pipeline.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace NanoAI_FLOW;

/// 确定性 CPU 密集核函数，用于模拟计算型推理阶段。
inline int cpu_spin_mix(int x)
{
    int acc = x;
    for (int i = 0; i < 200; ++i)
    {
        acc = (acc * 131 + i) ^ (acc >> 1);
    }
    return acc;
}

/**
 * @brief 计算阶段示例。
 * @tparam Policy 阶段执行策略。
 * @tparam Concurrency 阶段并发度。
 * @tparam DedicatedSize 专用线程池大小（仅 dedicated_pool 生效）。
 */
template <PipeExecutionPolicy Policy, nanoai_u32 Concurrency = 8, nanoai_u32 DedicatedSize = 0>
class ComputePipe final : public NanoPipe<ComputePipe<Policy, Concurrency, DedicatedSize>, Concurrency, Policy, DedicatedSize>
{
public:
    /// @param bias 阶段输出偏置，用于区分不同阶段。
    explicit ComputePipe(int bias) noexcept : bias_(bias) {}

    /// 模拟单个计算阶段。
    int on_run(int input)
    {
        const int mixed = cpu_spin_mix(input);
        return mixed + bias_;
    }

private:
    /// 当前阶段的输出偏置。
    int bias_;
};

/// 单个策略用例的聚合指标。
struct BenchResult
{
    /// 用例名称。
    std::string name;
    /// 总任务数。
    int jobs{0};
    /// 提交线程数。
    int submit_threads{0};
    /// 总耗时（毫秒）。
    double time_ms{0.0};
    /// 吞吐（每秒请求数）。
    double qps{0.0};
    /// 输出校验和，用于快速一致性观察。
    nanoai_u64 checksum{0};
    /// 输出值正确性标记。
    bool output_ok{false};
    /// 完成顺序是否单调。
    bool completion_order_monotonic{false};
    /// 完成顺序破坏次数。
    int completion_order_breaks{0};
};

/**
 * @brief 运行单个高并发基准用例。
 * @tparam PipelineFactory 可调用对象类型，返回已构建的流水线实例。
 */
template <typename PipelineFactory>
BenchResult run_high_concurrency_benchmark(
    const std::string &name,
    int jobs,
    int submit_threads,
    PipelineFactory make_pipeline)
{
    // 为当前用例构建一条强类型流水线实例。
    auto pipeline = make_pipeline();

    std::vector<int> outputs(static_cast<nanoai_usize>(jobs), 0);
    std::vector<int> completion_order;
    completion_order.reserve(static_cast<nanoai_usize>(jobs));
    std::mutex completion_mtx;

    std::atomic<int> next{0};
    std::atomic<nanoai_u64> checksum{0};

    const auto begin = std::chrono::steady_clock::now();

    std::vector<std::thread> workers;
    workers.reserve(static_cast<nanoai_usize>(submit_threads));
    for (int t = 0; t < submit_threads; ++t)
    {
        // 通过原子索引分发任务，近似 work-stealing 提交模式。
        workers.emplace_back([&]() {
            for (;;)
            {
                const int idx = next.fetch_add(1);
                if (idx >= jobs)
                {
                    break;
                }

                const int out = pipeline.run(idx);
                outputs[static_cast<nanoai_usize>(idx)] = out;
                checksum.fetch_add(static_cast<nanoai_u64>(out), std::memory_order_relaxed);

                {
                    std::lock_guard lock(completion_mtx);
                    completion_order.push_back(idx);
                }
            }
        });
    }

    for (auto &worker : workers)
    {
        worker.join();
    }

    const auto end = std::chrono::steady_clock::now();
    const double time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    const double qps = (time_ms > 0.0) ? (static_cast<double>(jobs) * 1000.0 / time_ms) : 0.0;

    bool output_ok = true;
    for (int i = 0; i < jobs; ++i)
    {
        // 三阶段组合公式对应的真值计算。
        const int expected = cpu_spin_mix(cpu_spin_mix(cpu_spin_mix(i) + 3) + 5) + 7;
        if (outputs[static_cast<nanoai_usize>(i)] != expected)
        {
            output_ok = false;
            break;
        }
    }

    int breaks = 0;
    for (nanoai_usize i = 1; i < completion_order.size(); ++i)
    {
        if (completion_order[i] < completion_order[i - 1])
        {
            ++breaks;
        }
    }

    BenchResult result;
    result.name = name;
    result.jobs = jobs;
    result.submit_threads = submit_threads;
    result.time_ms = time_ms;
    result.qps = qps;
    result.checksum = checksum.load(std::memory_order_relaxed);
    result.output_ok = output_ok;
    result.completion_order_monotonic = (breaks == 0);
    result.completion_order_breaks = breaks;
    return result;
}

/// 打印单个用例结果。
void print_bench_result(const BenchResult &result)
{
    std::cout << "[PERF] " << result.name
              << " jobs=" << result.jobs
              << " submit_threads=" << result.submit_threads
              << " time_ms=" << result.time_ms
              << " qps=" << std::fixed << std::setprecision(2) << result.qps
              << " checksum=" << result.checksum
              << " output_ok=" << (result.output_ok ? "PASS" : "FAIL")
              << '\n';

    std::cout << "[ORDER] " << result.name
              << " completion_monotonic=" << (result.completion_order_monotonic ? "YES" : "NO")
              << " breaks=" << result.completion_order_breaks
              << " (breaks > 0 usually means submit-side contention, not result error)"
              << '\n';
}

/// 计算相对基线的百分比变化，便于快速观察 profile 调优收益或退化幅度。
double pct_delta(double baseline, double current)
{
    if (baseline <= 0.0)
    {
        return 0.0;
    }
    return (current - baseline) * 100.0 / baseline;
}

int main()
{
    // 基准测试分为三段：
    // 1. shared/dedicated/mixed 的高压吞吐比较；
    // 2. ordered/unordered 固定模式比较；
    // 3. ordered 模式下不同 drain profile 的调优对比。
    // 这样可以把“执行策略差异”和“同一策略下的运行时参数差异”拆开观察。
    const int jobs = 20000;
    const int submit_threads = static_cast<int>(std::max(4u, std::thread::hardware_concurrency()));
    const int strict_order_jobs = 8000;

    std::cout << "========== High Concurrency Perf: shared / dedicated / mixed ==========" << '\n';
    std::cout << "[CONFIG] jobs=" << jobs << ", submit_threads=" << submit_threads << '\n';

    // shared_pool_all：观察所有阶段竞争同一共享池时的吞吐与排序行为。
    const auto shared_result = run_high_concurrency_benchmark(
        "shared_pool_all",
        jobs,
        submit_threads,
        []() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline<PipeForwardOrder::ordered>(16, 64, pipe1, pipe2, pipe3);
        });

    // dedicated_pool_all：观察完全资源隔离时的吞吐上限与线程成本。
    const auto dedicated_result = run_high_concurrency_benchmark(
        "dedicated_pool_all",
        jobs,
        submit_threads,
        []() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(7);
            return make_pipeline<PipeForwardOrder::ordered>(16, 64, pipe1, pipe2, pipe3);
        });

    // mixed_pool_shared_dedicated_shared：模拟真实业务里“重阶段单独隔离，其余阶段共享”的折中方案。
    const auto mixed_result = run_high_concurrency_benchmark(
        "mixed_pool_shared_dedicated_shared",
        jobs,
        submit_threads,
        []() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline<PipeForwardOrder::ordered>(16, 64, pipe1, pipe2, pipe3);
        });

    print_bench_result(shared_result);
    print_bench_result(dedicated_result);
    print_bench_result(mixed_result);

    std::cout << "\n========== Pipeline Fixed Modes (ordered/unordered) ==========" << '\n';

    const NanoPipeLineOptions mode_profile{16, 64};

    // ordered 与 unordered 对比主要回答：顺序确定性要付出多少吞吐代价。
    const auto mode_ordered = run_high_concurrency_benchmark(
        "mode_ordered",
        jobs,
        submit_threads,
        [&]() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline<PipeForwardOrder::ordered>(mode_profile, pipe1, pipe2, pipe3);
        });

    const auto mode_unordered = run_high_concurrency_benchmark(
        "mode_unordered",
        jobs,
        submit_threads,
        [&]() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline<PipeForwardOrder::unordered>(mode_profile, pipe1, pipe2, pipe3);
        });

    print_bench_result(mode_ordered);
    print_bench_result(mode_unordered);

    std::cout << "\n========== Ordered Adaptive Profiles (default/throughput/latency) ==========" << '\n';

    const NanoPipeLineOptions profile_default{16, 64};

    // throughput profile 倾向减少调度切换频率，以更大的批量换取吞吐。
    auto profile_throughput = profile_default;
    profile_throughput.drain_batch_min = 64;
    profile_throughput.drain_batch_max = 1024;
    profile_throughput.drain_fast_threshold_us = 120;
    profile_throughput.drain_slow_threshold_us = 500;
    profile_throughput.yield_on_full_batch = false;
    profile_throughput.yield_on_slow_batch = true;

    // latency profile 倾向更快让出执行权，以降低单次 drain 过长导致的尾延时风险。
    auto profile_latency = profile_default;
    profile_latency.drain_batch_min = 4;
    profile_latency.drain_batch_max = 128;
    profile_latency.drain_fast_threshold_us = 40;
    profile_latency.drain_slow_threshold_us = 180;
    profile_latency.yield_on_full_batch = true;
    profile_latency.yield_on_slow_batch = true;

    const auto profile_default_result = run_high_concurrency_benchmark(
        "profile_default_ordered",
        jobs,
        submit_threads,
        [&]() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline<PipeForwardOrder::ordered>(profile_default, pipe1, pipe2, pipe3);
        });

    const auto profile_throughput_result = run_high_concurrency_benchmark(
        "profile_throughput_ordered",
        jobs,
        submit_threads,
        [&]() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline<PipeForwardOrder::ordered>(profile_throughput, pipe1, pipe2, pipe3);
        });

    const auto profile_latency_result = run_high_concurrency_benchmark(
        "profile_latency_ordered",
        jobs,
        submit_threads,
        [&]() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline<PipeForwardOrder::ordered>(profile_latency, pipe1, pipe2, pipe3);
        });

    print_bench_result(profile_default_result);
    print_bench_result(profile_throughput_result);
    print_bench_result(profile_latency_result);

    std::cout << "[PROFILE-DELTA] throughput_vs_default"
              << " ordered=" << std::fixed << std::setprecision(2) << pct_delta(profile_default_result.qps, profile_throughput_result.qps) << "%"
              << '\n';

    std::cout << "[PROFILE-DELTA] latency_vs_default"
              << " ordered=" << std::fixed << std::setprecision(2) << pct_delta(profile_default_result.qps, profile_latency_result.qps) << "%"
              << '\n';

    std::cout << "\n========== Strict Order Control (single submit thread) ==========" << '\n';
    // 最后一段使用单提交线程做对照，尽量减少 submit 侧竞争对“完成顺序”观测的污染。
    std::cout << "[CONFIG] jobs=" << strict_order_jobs << ", submit_threads=1" << '\n';

    const auto shared_strict = run_high_concurrency_benchmark(
        "shared_pool_all_strict_order_control",
        strict_order_jobs,
        1,
        []() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline<PipeForwardOrder::ordered>(16, 64, pipe1, pipe2, pipe3);
        });

    const auto dedicated_strict = run_high_concurrency_benchmark(
        "dedicated_pool_all_strict_order_control",
        strict_order_jobs,
        1,
        []() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(7);
            return make_pipeline<PipeForwardOrder::ordered>(16, 64, pipe1, pipe2, pipe3);
        });

    const auto mixed_strict = run_high_concurrency_benchmark(
        "mixed_pool_strict_order_control",
        strict_order_jobs,
        1,
        []() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline<PipeForwardOrder::ordered>(16, 64, pipe1, pipe2, pipe3);
        });

    print_bench_result(shared_strict);
    print_bench_result(dedicated_strict);
    print_bench_result(mixed_strict);

    std::cout << "[STRICT-CHECK] shared_pool=" << (shared_strict.completion_order_monotonic ? "PASS" : "FAIL") << '\n';
    std::cout << "[STRICT-CHECK] dedicated_pool=" << (dedicated_strict.completion_order_monotonic ? "PASS" : "FAIL") << '\n';
    std::cout << "[STRICT-CHECK] mixed_pool=" << (mixed_strict.completion_order_monotonic ? "PASS" : "FAIL") << '\n';

    return 0;
}
