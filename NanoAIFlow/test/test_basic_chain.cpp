// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: test_basic_chain.cpp
// Brief: 验证基础线性流水线算子链路行为。

#include <nanoai_flow/nanoai_flow.hpp>

#include <iostream>

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

    bool ok() const
    {
        return actual == expected;
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
              << '\n';
    std::cout << "[TEST] basic_chain=" << (ok ? "PASS" : "FAIL") << '\n';
    return ok ? 0 : 1;
}
