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

inline int cpu_spin_mix(int x)
{
    int acc = x;
    for (int i = 0; i < 200; ++i)
    {
        acc = (acc * 131 + i) ^ (acc >> 1);
    }
    return acc;
}

template <PipeExecutionPolicy Policy, PipeCount Concurrency = 8, PipeCount DedicatedSize = 0>
class ComputePipe final : public NanoPipe<ComputePipe<Policy, Concurrency, DedicatedSize>, Concurrency, Policy, DedicatedSize>
{
public:
    explicit ComputePipe(int bias) noexcept : bias_(bias) {}

    int on_run(int input)
    {
        const int mixed = cpu_spin_mix(input);
        return mixed + bias_;
    }

private:
    int bias_;
};

struct BenchResult
{
    std::string name;
    int jobs{0};
    int submit_threads{0};
    double time_ms{0.0};
    double qps{0.0};
    std::uint64_t checksum{0};
    bool output_ok{false};
    bool completion_order_monotonic{false};
    int completion_order_breaks{0};
};

template <typename PipelineFactory>
BenchResult run_high_concurrency_benchmark(
    const std::string &name,
    int jobs,
    int submit_threads,
    PipelineFactory make_pipeline)
{
    auto pipeline = make_pipeline();

    std::vector<int> outputs(static_cast<std::size_t>(jobs), 0);
    std::vector<int> completion_order;
    completion_order.reserve(static_cast<std::size_t>(jobs));
    std::mutex completion_mtx;

    std::atomic<int> next{0};
    std::atomic<std::uint64_t> checksum{0};

    const auto begin = std::chrono::steady_clock::now();

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(submit_threads));
    for (int t = 0; t < submit_threads; ++t)
    {
        workers.emplace_back([&]() {
            for (;;)
            {
                const int idx = next.fetch_add(1);
                if (idx >= jobs)
                {
                    break;
                }

                const int out = pipeline.run(idx);
                outputs[static_cast<std::size_t>(idx)] = out;
                checksum.fetch_add(static_cast<std::uint64_t>(out), std::memory_order_relaxed);

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
        const int expected = cpu_spin_mix(cpu_spin_mix(cpu_spin_mix(i) + 3) + 5) + 7;
        if (outputs[static_cast<std::size_t>(i)] != expected)
        {
            output_ok = false;
            break;
        }
    }

    int breaks = 0;
    for (std::size_t i = 1; i < completion_order.size(); ++i)
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

int main()
{
    const int jobs = 20000;
    const int submit_threads = static_cast<int>(std::max(4u, std::thread::hardware_concurrency()));
    const int strict_order_jobs = 8000;

    std::cout << "========== High Concurrency Perf: shared / dedicated / mixed ==========" << '\n';
    std::cout << "[CONFIG] jobs=" << jobs << ", submit_threads=" << submit_threads << '\n';

    const auto shared_result = run_high_concurrency_benchmark(
        "shared_pool_all",
        jobs,
        submit_threads,
        []() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline_builder(16, 64).add_pipe(pipe1).add_pipe(pipe2).add_pipe(pipe3).build();
        });

    const auto dedicated_result = run_high_concurrency_benchmark(
        "dedicated_pool_all",
        jobs,
        submit_threads,
        []() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(7);
            return make_pipeline_builder(16, 64).add_pipe(pipe1).add_pipe(pipe2).add_pipe(pipe3).build();
        });

    const auto mixed_result = run_high_concurrency_benchmark(
        "mixed_pool_shared_dedicated_shared",
        jobs,
        submit_threads,
        []() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline_builder(16, 64).add_pipe(pipe1).add_pipe(pipe2).add_pipe(pipe3).build();
        });

    print_bench_result(shared_result);
    print_bench_result(dedicated_result);
    print_bench_result(mixed_result);

    std::cout << "\n========== Strict Order Control (single submit thread) ==========" << '\n';
    std::cout << "[CONFIG] jobs=" << strict_order_jobs << ", submit_threads=1" << '\n';

    const auto shared_strict = run_high_concurrency_benchmark(
        "shared_pool_all_strict_order_control",
        strict_order_jobs,
        1,
        []() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline_builder(16, 64).add_pipe(pipe1).add_pipe(pipe2).add_pipe(pipe3).build();
        });

    const auto dedicated_strict = run_high_concurrency_benchmark(
        "dedicated_pool_all_strict_order_control",
        strict_order_jobs,
        1,
        []() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(7);
            return make_pipeline_builder(16, 64).add_pipe(pipe1).add_pipe(pipe2).add_pipe(pipe3).build();
        });

    const auto mixed_strict = run_high_concurrency_benchmark(
        "mixed_pool_strict_order_control",
        strict_order_jobs,
        1,
        []() {
            auto pipe1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto pipe2 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(5);
            auto pipe3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline_builder(16, 64).add_pipe(pipe1).add_pipe(pipe2).add_pipe(pipe3).build();
        });

    print_bench_result(shared_strict);
    print_bench_result(dedicated_strict);
    print_bench_result(mixed_strict);

    std::cout << "[STRICT-CHECK] shared_pool=" << (shared_strict.completion_order_monotonic ? "PASS" : "FAIL") << '\n';
    std::cout << "[STRICT-CHECK] dedicated_pool=" << (dedicated_strict.completion_order_monotonic ? "PASS" : "FAIL") << '\n';
    std::cout << "[STRICT-CHECK] mixed_pool=" << (mixed_strict.completion_order_monotonic ? "PASS" : "FAIL") << '\n';

    return 0;
}
