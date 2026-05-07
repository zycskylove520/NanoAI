// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: test_reference.cpp
// Brief: 验证可变引用在流水线阶段间的传播与副作用。

#include <nanoai_flow/nanoai_flow.hpp>

#include <iostream>

using namespace NanoAI_FLOW;

// 文件用途：
// 验证流水线在“引用类型输入”场景下能正确传递并修改原始对象。
namespace
{

/// 阶段 1：通过非常量引用修改输入。
class IncrementByRefPipe : public NanoPipe<IncrementByRefPipe>
{
public:
    int on_run(int &value)
    {
        value += 2;
        return value;
    }
};

/// 阶段 2：在修改后执行简单算术变换。
class MinusPipe : public NanoPipe<MinusPipe>
{
public:
    int on_run(int value)
    {
        return value - 1;
    }
};

/// 测试结果结构体，包含输出与副作用检查。
struct ReferenceResult
{
    /// 调用前输入值。
    int input_before{0};
    /// 调用后输入值（用于验证引用副作用）。
    int input_after{0};
    /// 期望输出值。
    int expected_output{0};
    /// 实际输出值。
    int actual_output{0};

    bool ok() const
    {
        return actual_output == expected_output && input_after == 12;
    }
};

ReferenceResult test_reference_input()
{
    // 预期链路: 10 --(ref +2)--> 12 --(-1)--> 11。
    // 同时校验外部变量确实被引用语义修改为 12。
    IncrementByRefPipe inc;
    MinusPipe minus;
    NanoPipeLine pipeline(4, 16, inc, minus);

    int x = 10;
    ReferenceResult result;
    result.input_before = x;
    result.expected_output = 11;
    result.actual_output = pipeline.run(x);
    result.input_after = x;
    return result;
}

} // namespace

int main()
{
    // CI/CTest 返回码约定：
    // 0 表示通过，非 0 表示失败。
    const auto result = test_reference_input();
    const bool ok = result.ok();

    std::cout << "[RESULT] reference_input"
              << " input_before=" << result.input_before
              << " input_after=" << result.input_after
              << " expected_output=" << result.expected_output
              << " actual_output=" << result.actual_output
              << '\n';
    std::cout << "[TEST] reference_input=" << (ok ? "PASS" : "FAIL") << '\n';
    return ok ? 0 : 1;
}
