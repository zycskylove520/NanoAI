#include <nanoai_flow/nanoai_flow.hpp>

#include <iostream>

using namespace NanoAI_FLOW;

// 文件用途：
// 验证最基础的串行流水线行为：
// 输入 -> AddOnePipe -> DoublePipe，并检查最终数值结果。
namespace
{

class AddOnePipe : public NanoPipe<AddOnePipe>
{
public:
    int on_run(int x)
    {
        return x + 1;
    }
};

class DoublePipe : public NanoPipe<DoublePipe>
{
public:
    int on_run(int x)
    {
        return x * 2;
    }
};

struct BasicChainResult
{
    int input{0};
    int expected{0};
    int actual{0};

    bool ok() const
    {
        return actual == expected;
    }
};

BasicChainResult test_basic_chain()
{
    // 功能预期：
    // 5 -> (5 + 1) -> (6 * 2) = 12。
    AddOnePipe add;
    DoublePipe dbl;
    NanoPipeLine pipeline(4, 16, add, dbl);

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
