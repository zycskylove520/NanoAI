// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: test_load_once_concurrency.cpp
// Brief: 验证并发争用下模型仅加载一次且输出正确。

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

/// 加载阶段示例：load_model 理应仅被调用一次。
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

/// 模型加载之后的推理阶段示例。
class DemoInfer final : public InferModelPipe<DemoInfer, 4, PipeExecutionPolicy::shared_pool>
{
public:
    int infer(int x)
    {
        return x + 100;
    }
};

/// 并发“仅加载一次”验证结果。
struct LoadOnceResult
{
    /// 总任务数。
    int jobs{0};
    /// 工作线程数。
    int worker_count{0};
    /// 实际加载次数，期望为 1。
    int load_count{0};
    /// 首个不匹配索引，-1 表示无错误。
    int mismatch_index{-1};
    /// 不匹配位置的期望值。
    int expected_at_mismatch{0};
    /// 不匹配位置的实际值。
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
    auto pipeline = make_pipeline<PipeForwardOrder::ordered>(4, 32, std::ref(loader), infer);

    // 并行执行大量任务，压测“仅加载一次模型”语义。
    constexpr int kJobs = 200;
    std::atomic<int> next{0};
    std::vector<int> outputs(static_cast<nanoai_usize>(kJobs), 0);

    std::vector<std::thread> workers;
    const int worker_count = static_cast<int>(std::max(4u, std::thread::hardware_concurrency()));
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

    LoadOnceResult result;
    result.jobs = kJobs;
    result.worker_count = worker_count;
    result.load_count = loader.load_count.load(std::memory_order_relaxed);

    // 每个输出都应满足 infer(x) = x + 100，不依赖执行顺序。
    for (int i = 0; i < kJobs; ++i)
    {
        const int expected = i + 100;
        const int actual = outputs[static_cast<nanoai_usize>(i)];
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
