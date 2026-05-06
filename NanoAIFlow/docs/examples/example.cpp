#include "core/pipeline.hpp"

#include <iostream>
#include <string>
#include <tuple>

using namespace NanoAI_FLOW;

class AddOnePipe final : public NanoPipe<AddOnePipe>
{
public:
    int on_run(int x)
    {
        return x + 1;
    }
};

class ToPairPipe final : public NanoPipe<ToPairPipe>
{
public:
    std::tuple<int, std::string> on_run(int x)
    {
        return {x, "stage2"};
    }
};

class JoinPipe final : public NanoPipe<JoinPipe>
{
public:
    std::string on_run(int value, const std::string &tag)
    {
        return tag + ":" + std::to_string(value);
    }
};

template <PipeExecutionPolicy Policy>
class PolicyPipe final : public NanoPipe<PolicyPipe<Policy>, 4, Policy>
{
public:
    int on_run(int x)
    {
        return x * 2;
    }
};

int main()
{
    // 1) 直接构造固定节点管线。
    AddOnePipe p1;
    ToPairPipe p2;
    JoinPipe p3;
    NanoPipeLine pipeline(8, 64, p1, p2, p3);

    const std::string out = pipeline.run(10);
    std::cout << "pipeline output: " << out << std::endl;

    // 2) 使用 builder 链式构建。
    auto pipeline2 = make_pipeline_builder(8, 64)
                         .add_pipe(PolicyPipe<PipeExecutionPolicy::shared_pool>{})
                         .add_pipe(PolicyPipe<PipeExecutionPolicy::dedicated_pool>{})
                         .build();

    const int out2 = pipeline2.run(3);
    std::cout << "pipeline2 output: " << out2 << std::endl;

    return 0;
}
