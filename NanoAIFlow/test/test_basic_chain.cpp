// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: test_basic_chain.cpp
// Brief: 验证基础线性流水线算子链路行为。

#include <nanoai_flow/nanoai_flow.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

using namespace NanoAI_FLOW;

// 文件用途：
// 验证最基础的串行流水线行为：
// 输入 -> AddOnePipe -> DoublePipe，并检查最终数值结果。
namespace
{

/// 阶段 1：x -> x + 1。
class AddOnePipe : public NanoPipe<AddOnePipe>
{
public:
    int on_run(int x)
    {
        return x + 1;
    }
};

/// 阶段 2：x -> x * 2。
class DoublePipe : public NanoPipe<DoublePipe>
{
public:
    int on_run(int x)
    {
        return x * 2;
    }
};

/// 测试结果结构体，用于断言与输出。
struct BasicChainResult
{
    /// 输入值。
    int input{0};
    /// 期望输出。
    int expected{0};
    /// 实际输出。
    int actual{0};
    /// 拷贝构造后输出。
    int copied_actual{0};
    /// 移动构造后输出。
    int moved_actual{0};
    /// 编译期拷贝构造能力。
    bool copy_constructible{false};
    /// 编译期移动构造能力。
    bool move_constructible{false};
    /// 超时关闭是否在期限内完成。
    bool shutdown_for_ok{false};
    /// 关闭后是否正确拒绝新任务。
    bool rejects_after_shutdown{false};

    bool ok() const
    {
        return actual == expected && copied_actual == expected && moved_actual == expected &&
               copy_constructible && move_constructible && shutdown_for_ok && rejects_after_shutdown;
    }
};

BasicChainResult test_basic_chain()
{
    // 预期链路: 5 -> AddOnePipe -> DoublePipe = 12。
    AddOnePipe add;
    DoublePipe dbl;
    auto pipeline = make_pipeline<PipeForwardOrder::ordered>(4, 16, add, dbl);

    BasicChainResult result;
    result.input = 5;
    result.expected = 12;
    result.actual = pipeline.run(result.input);

    using PipelineT = decltype(pipeline);
    result.copy_constructible = std::is_copy_constructible_v<PipelineT>;
    result.move_constructible = std::is_move_constructible_v<PipelineT>;

    auto copied_pipeline = pipeline;
    result.copied_actual = copied_pipeline.run(result.input);

    auto moved_pipeline = std::move(copied_pipeline);
    result.moved_actual = moved_pipeline.run(result.input);

    result.shutdown_for_ok = moved_pipeline.shutdown_for(std::chrono::milliseconds(200));

    try
    {
        (void)moved_pipeline.run(result.input);
        result.rejects_after_shutdown = false;
    }
    catch (const std::runtime_error &)
    {
        result.rejects_after_shutdown = true;
    }

    return result;
}

} // namespace

int main()
{
    // CI/CTest 返回码约定：
    // 0 表示通过，非 0 表示失败。
    const auto result = test_basic_chain();
    const bool ok = result.ok();

    std::cout << "[RESULT] basic_chain"
              << " input=" << result.input
              << " expected=" << result.expected
              << " actual=" << result.actual
              << " copied_actual=" << result.copied_actual
              << " moved_actual=" << result.moved_actual
              << " copy_constructible=" << (result.copy_constructible ? "YES" : "NO")
              << " move_constructible=" << (result.move_constructible ? "YES" : "NO")
              << " shutdown_for_ok=" << (result.shutdown_for_ok ? "YES" : "NO")
              << " rejects_after_shutdown=" << (result.rejects_after_shutdown ? "YES" : "NO")
              << '\n';
    std::cout << "[TEST] basic_chain=" << (ok ? "PASS" : "FAIL") << '\n';
    return ok ? 0 : 1;
}
