#include <nanoai_flow/nanoai_flow.hpp>

#include <atomic>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

using namespace NanoAI_FLOW;

// 文件用途：
// 验证并发请求下的两个行为：
// 1) LoadModelPipe 中模型加载只发生一次，
// 2) 多线程执行下推理结果保持正确。
namespace
{

class DemoLoader final : public LoadModelPipe<DemoLoader, 2, PipeExecutionPolicy::shared_pool>
{
public:
    int load_model(int) const
    {
        load_count.fetch_add(1, std::memory_order_relaxed);
        return 0;
    }

    mutable std::atomic<int> load_count{0};
};

class DemoInfer final : public InferModelPipe<DemoInfer, 4, PipeExecutionPolicy::shared_pool>
{
public:
    int infer(int x)
    {
        return x + 100;
    }
};

struct LoadOnceResult
{
    int jobs{0};
    int worker_count{0};
    int load_count{0};
    int mismatch_index{-1};
    int expected_at_mismatch{0};
    int actual_at_mismatch{0};

    bool ok() const
    {
        return load_count == 1 && mismatch_index < 0;
    }
};

LoadOnceResult test_load_once_with_concurrency()
{
    DemoLoader loader;
    DemoInfer infer;
    NanoPipeLine pipeline(4, 32, std::ref(loader), infer);

    // 并行执行大量任务，压测“仅加载一次模型”的行为。
    constexpr int kJobs = 200;
    std::atomic<int> next{0};
    std::vector<int> outputs(static_cast<std::size_t>(kJobs), 0);

    std::vector<std::thread> workers;
    const int worker_count = static_cast<int>(std::max(4u, std::thread::hardware_concurrency()));
    workers.reserve(static_cast<std::size_t>(worker_count));

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
                outputs[static_cast<std::size_t>(idx)] = pipeline.run(idx);
            }
        });
    }

    for (auto &w : workers)
    {
        w.join();
    }

    LoadOnceResult result;
    result.jobs = kJobs;
    result.worker_count = worker_count;
    result.load_count = loader.load_count.load(std::memory_order_relaxed);

    // 每个输出都应满足 infer(x) = x + 100。
    for (int i = 0; i < kJobs; ++i)
    {
        const int expected = i + 100;
        const int actual = outputs[static_cast<std::size_t>(i)];
        if (actual != expected)
        {
            result.mismatch_index = i;
            result.expected_at_mismatch = expected;
            result.actual_at_mismatch = actual;
            break;
        }
    }

    return result;
}

} // namespace

int main()
{
    // CI/CTest 返回码约定：
    // 0 表示通过，非 0 表示失败。
    const auto result = test_load_once_with_concurrency();
    const bool ok = result.ok();

    std::cout << "[RESULT] load_once_concurrency"
              << " jobs=" << result.jobs
              << " worker_count=" << result.worker_count
              << " load_count=" << result.load_count
              << " mismatch_index=" << result.mismatch_index
              << '\n';
    if (result.mismatch_index >= 0)
    {
        std::cout << "[DETAIL] mismatch"
                  << " index=" << result.mismatch_index
                  << " expected=" << result.expected_at_mismatch
                  << " actual=" << result.actual_at_mismatch
                  << '\n';
    }
    std::cout << "[TEST] load_once_concurrency=" << (ok ? "PASS" : "FAIL") << '\n';
    return ok ? 0 : 1;
}
