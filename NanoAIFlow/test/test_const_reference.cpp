#include <nanoai_flow/nanoai_flow.hpp>

#include <iostream>

using namespace NanoAI_FLOW;

// 文件用途：
// 验证流水线支持“常量引用输入”，并保证输入对象不被修改。
namespace
{

struct Sample
{
    int a{0};
    int b{0};
};

class SumByConstRefPipe : public NanoPipe<SumByConstRefPipe>
{
public:
    int on_run(const Sample &s) const
    {
        return s.a + s.b;
    }
};

class PlusPipe : public NanoPipe<PlusPipe>
{
public:
    int on_run(int value)
    {
        return value + 5;
    }
};

struct ConstReferenceResult
{
    int input_a{0};
    int input_b{0};
    int expected{0};
    int actual{0};
    bool input_unchanged{false};

    bool ok() const
    {
        return actual == expected && input_unchanged;
    }
};

ConstReferenceResult test_const_reference_input()
{
    // 功能预期：
    // (4 + 6) + 5 = 15，且源对象保持不变。
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
