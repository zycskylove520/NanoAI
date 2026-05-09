// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: test_tuple_forwarding.cpp
// Brief: 验证相邻阶段间 tuple 输出展开转发能力。

#include <nanoai_flow/nanoai_flow.hpp>

#include <iostream>
#include <tuple>

using namespace NanoAI_FLOW;

// 文件用途：
// 验证流水线阶段之间的 tuple 转发能力。
// 第 1 阶段返回 tuple，第 2 阶段将 tuple 元素作为参数消费。
namespace
{

/// 阶段 1：把标量转换为二元组。
class TuplePackPipe : public NanoPipe<TuplePackPipe>
{
public:
    std::tuple<int, int> on_run(int x)
    {
        return {x, x + 10};
    }
};

/// 阶段 2：消费 tuple 展开后的参数。
class TupleConsumePipe : public NanoPipe<TupleConsumePipe>
{
public:
    int on_run(int x, int y)
    {
        return x + y;
    }
};

/// 测试结果结构体，用于断言与输出。
struct TupleForwardingResult
{
    /// 输入值。
    int input{0};
    /// 期望输出。
    int expected{0};
    /// 实际输出。
    int actual{0};

    bool ok() const
    {
        return actual == expected;
    }
};

TupleForwardingResult test_tuple_forwarding()
{
    // 预期链路: 3 -> (3, 13) -> 16。
    auto pipeline = make_pipeline<PipeForwardOrder::ordered>(
        4,
        16,
        TuplePackPipe{},
        TupleConsumePipe{});

    TupleForwardingResult result;
    result.input = 3;
    result.expected = 16;
    result.actual = pipeline.run(result.input);
    return result;
}

} // namespace

int main()
{
    // CI/CTest 返回码约定：
    // 0 表示通过，非 0 表示失败。
    const auto result = test_tuple_forwarding();
    const bool ok = result.ok();

    std::cout << "[RESULT] tuple_forwarding"
              << " input=" << result.input
              << " expected=" << result.expected
              << " actual=" << result.actual
              << '\n';
    std::cout << "[TEST] tuple_forwarding=" << (ok ? "PASS" : "FAIL") << '\n';
    return ok ? 0 : 1;
}
