#include <nanoai_flow/nanoai_flow.hpp>

#include <iostream>

using namespace NanoAI_FLOW;

// 文件用途：
// 验证流水线可以正确处理“指针类型输入”，并保证跨阶段传递结果正确。
namespace
{

class ReadPointerPipe : public NanoPipe<ReadPointerPipe>
{
public:
    int on_run(int *value)
    {
        if (value == nullptr)
        {
            return -1;
        }
        return *value + 1;
    }
};

class MultiplyPipe : public NanoPipe<MultiplyPipe>
{
public:
    int on_run(int value)
    {
        return value * 3;
    }
};

struct PointerResult
{
    int input{0};
    int expected{0};
    int actual{0};

    bool ok() const
    {
        return actual == expected;
    }
};

PointerResult test_pointer_input()
{
    // 功能预期：
    // 输入指针指向 7 -> 读取后 +1 = 8 -> *3 = 24。
    ReadPointerPipe read;
    MultiplyPipe mul;
    NanoPipeLine pipeline(4, 16, read, mul);

    int x = 7;
    PointerResult result;
    result.input = x;
    result.expected = 24;
    result.actual = pipeline.run(&x);
    return result;
}

} // namespace

int main()
{
    // CI/CTest 返回码约定：
    // 0 表示通过，非 0 表示失败。
    const auto result = test_pointer_input();
    const bool ok = result.ok();

    std::cout << "[RESULT] pointer_input"
              << " input=" << result.input
              << " expected=" << result.expected
              << " actual=" << result.actual
              << '\n';
    std::cout << "[TEST] pointer_input=" << (ok ? "PASS" : "FAIL") << '\n';
    return ok ? 0 : 1;
}
