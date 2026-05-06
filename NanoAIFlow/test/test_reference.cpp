#include <nanoai_flow/nanoai_flow.hpp>

#include <iostream>

using namespace NanoAI_FLOW;

// 文件用途：
// 验证流水线在“引用类型输入”场景下能正确传递并修改原始对象。
namespace
{

class IncrementByRefPipe : public NanoPipe<IncrementByRefPipe>
{
public:
    int on_run(int &value)
    {
        value += 2;
        return value;
    }
};

class MinusPipe : public NanoPipe<MinusPipe>
{
public:
    int on_run(int value)
    {
        return value - 1;
    }
};

struct ReferenceResult
{
    int input_before{0};
    int input_after{0};
    int expected_output{0};
    int actual_output{0};

    bool ok() const
    {
        return actual_output == expected_output && input_after == 12;
    }
};

ReferenceResult test_reference_input()
{
    // 功能预期：
    // 原始值 10 经引用阶段修改为 12，再减 1 得到 11。
    // 同时校验外部变量确实被引用语义修改。
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
