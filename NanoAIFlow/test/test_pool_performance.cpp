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

inline int cpu_spin_mix(int x)
{
    int acc = x;
    for (int i = 0; i < 120; ++i)
    {
        acc = (acc * 131 + i) ^ (acc >> 1);
    }
    return acc;
}

template <PipeExecutionPolicy Policy, PipeCount Concurrency = 8, PipeCount DedicatedSize = 0>
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
    int bias_;
};

struct PerfResult
{
    std::string name;
    int jobs{0};
    int submit_threads{0};
    double time_ms{0.0};
    double qps{0.0};
    std::uint64_t checksum{0};
    bool output_ok{false};
};

template <typename PipelineFactory>
PerfResult run_perf_case(const std::string &name, int jobs, int submit_threads, PipelineFactory make_pipeline)
{
    auto pipeline = make_pipeline();

    std::vector<int> outputs(static_cast<std::size_t>(jobs), 0);
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
                const int idx = next.fetch_add(1, std::memory_order_relaxed);
                if (idx >= jobs)
                {
                    break;
                }
                const int out = pipeline.run(idx);
                outputs[static_cast<std::size_t>(idx)] = out;
                checksum.fetch_add(static_cast<std::uint64_t>(out), std::memory_order_relaxed);
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
        const int expected = cpu_spin_mix(cpu_spin_mix(cpu_spin_mix(i) + 3) + 5) + 7;
        if (outputs[static_cast<std::size_t>(i)] != expected)
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
    // 这里不使用固定性能阈值，以避免不同机器环境下误报。
    // 测试通过条件：三种策略结果都正确，并成功输出性能统计。
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
            return make_pipeline_builder(16, 64).add_pipe(p1).add_pipe(p2).add_pipe(p3).build();
        });

    const auto dedicated = run_perf_case(
        "dedicated_pool_all",
        jobs,
        submit_threads,
        []() {
            auto p1 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(3);
            auto p2 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(5);
            auto p3 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(7);
            return make_pipeline_builder(16, 64).add_pipe(p1).add_pipe(p2).add_pipe(p3).build();
        });

    const auto mixed = run_perf_case(
        "mixed_pool_shared_dedicated_shared",
        jobs,
        submit_threads,
        []() {
            auto p1 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(3);
            auto p2 = ComputePipe<PipeExecutionPolicy::dedicated_pool, 8, 4>(5);
            auto p3 = ComputePipe<PipeExecutionPolicy::shared_pool, 8>(7);
            return make_pipeline_builder(16, 64).add_pipe(p1).add_pipe(p2).add_pipe(p3).build();
        });

    print_result(shared);
    print_result(dedicated);
    print_result(mixed);

    const int pass_cases = static_cast<int>(shared.output_ok) + static_cast<int>(dedicated.output_ok) + static_cast<int>(mixed.output_ok);
    std::cout << "[RESULT] pool_performance"
              << " pass_cases=" << pass_cases
              << "/3"
              << " jobs=" << jobs
              << " submit_threads=" << submit_threads
              << '\n';

    return shared.output_ok && dedicated.output_ok && mixed.output_ok;
}

} // namespace

int main()
{
    // CI/CTest 返回码约定：
    // 0 表示通过，非 0 表示失败。
    const bool ok = test_pool_performance_and_correctness();
    std::cout << "[TEST] pool_performance=" << (ok ? "PASS" : "FAIL") << '\n';
    return ok ? 0 : 1;
}
