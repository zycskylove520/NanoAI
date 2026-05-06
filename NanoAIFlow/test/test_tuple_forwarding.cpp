#include <nanoai_flow/nanoai_flow.hpp>

#include <iostream>
#include <tuple>

using namespace NanoAI_FLOW;

// 文件用途：
// 验证流水线阶段之间的 tuple 转发能力。
// 第 1 阶段返回 tuple，第 2 阶段将 tuple 元素作为参数消费。
namespace
{

class TuplePackPipe : public NanoPipe<TuplePackPipe>
{
public:
    std::tuple<int, int> on_run(int x)
    {
        return {x, x + 10};
    }
};

class TupleConsumePipe : public NanoPipe<TupleConsumePipe>
{
public:
    int on_run(int x, int y)
    {
        return x + y;
    }
};

struct TupleForwardingResult
{
    int input{0};
    int expected{0};
    int actual{0};

    bool ok() const
    {
        return actual == expected;
    }
};

TupleForwardingResult test_tuple_forwarding()
{
    // 功能预期：
    // 输入 3 -> TuplePackPipe 返回 (3, 13)
    // -> TupleConsumePipe 计算 3 + 13 = 16。
    auto pipeline = make_pipeline_builder(4, 16)
                        .add_pipe(TuplePackPipe{})
                        .add_pipe(TupleConsumePipe{})
                        .build();

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
