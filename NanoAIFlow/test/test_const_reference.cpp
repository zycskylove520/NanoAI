// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: test_const_reference.cpp
// Brief: 验证常量引用输入可被消费且不会被修改。

#include <nanoai_flow/nanoai_flow.hpp>

#include <iostream>

using namespace NanoAI_FLOW;

// 文件用途：
// 验证流水线支持“常量引用输入”，并保证输入对象不被修改。
namespace
{

/// 常量引用输入样例结构体。
struct Sample
{
    /// 字段 a。
    int a{0};
    /// 字段 b。
    int b{0};
};

/// 阶段 1：读取常量引用并求和。
class SumByConstRefPipe : public NanoPipe<SumByConstRefPipe>
{
public:
    int on_run(const Sample &s) const
    {
        return s.a + s.b;
    }
};

/// 阶段 2：增加固定偏置。
class PlusPipe : public NanoPipe<PlusPipe>
{
public:
    int on_run(int value)
    {
        return value + 5;
    }
};

/// 测试结果结构体，包含输出与不可变性检查。
struct ConstReferenceResult
{
    /// 输入 a。
    int input_a{0};
    /// 输入 b。
    int input_b{0};
    /// 期望输出。
    int expected{0};
    /// 实际输出。
    int actual{0};
    /// 输入对象是否保持不变。
    bool input_unchanged{false};

    bool ok() const
    {
        return actual == expected && input_unchanged;
    }
};

ConstReferenceResult test_const_reference_input()
{
    // 预期链路: (4 + 6) + 5 = 15，且源对象保持不变。
    SumByConstRefPipe sum;
    PlusPipe plus;
    NanoPipeLine pipeline(4, 16, sum, plus);

    const Sample s{4, 6};

    ConstReferenceResult result;
    result.input_a = s.a;
    result.input_b = s.b;
    result.expected = 15;
    result.actual = pipeline.run(s);
    result.input_unchanged = (s.a == result.input_a && s.b == result.input_b);
    return result;
}

} // namespace

int main()
{
    // CI/CTest 返回码约定：
    // 0 表示通过，非 0 表示失败。
    const auto result = test_const_reference_input();
    const bool ok = result.ok();

    std::cout << "[RESULT] const_reference_input"
              << " input_a=" << result.input_a
              << " input_b=" << result.input_b
              << " expected=" << result.expected
              << " actual=" << result.actual
              << " input_unchanged=" << (result.input_unchanged ? "YES" : "NO")
              << '\n';
    std::cout << "[TEST] const_reference_input=" << (ok ? "PASS" : "FAIL") << '\n';
    return ok ? 0 : 1;
}
