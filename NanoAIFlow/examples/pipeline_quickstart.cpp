// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: pipeline_quickstart.cpp
// Brief: 编译期固定顺序模式的最小示例。

#include "../include/nanoai_flow/core/pipeline.hpp"

#include <iostream>
#include <string>
#include <tuple>
#include <utility>

using namespace NanoAI_FLOW;

/// 阶段 1：输入加一。
class AddOnePipe final : public NanoPipe<AddOnePipe>
{
public:
    int on_run(int x)
    {
        return x + 1;
    }
};

/// 阶段 2：把标量拆分为二元组，供下一阶段按参数展开消费。
class ToPairPipe final : public NanoPipe<ToPairPipe>
{
public:
    std::tuple<int, std::string> on_run(int x)
    {
        return {x, "stage2"};
    }
};

/// 阶段 3：消费展开后的参数并拼接字符串结果。
class JoinPipe final : public NanoPipe<JoinPipe>
{
public:
    std::string on_run(int value, const std::string &tag)
    {
        return tag + ":" + std::to_string(value);
    }
};

/**
 * @brief 带策略模板参数的示例阶段。
 * @tparam Policy 阶段执行策略。
 */
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
    // 1) 直接构造编译期固定模式管线。
    // 适合节点集合在编译期已知、且不需要在运行时按条件裁剪阶段的主流程代码。
    AddOnePipe p1;
    ToPairPipe p2;
    JoinPipe p3;
    auto pipeline = make_pipeline<PipeForwardOrder::ordered>(8, 64, p1, p2, p3);

    const std::string out = pipeline.run(10);
    std::cout << "pipeline output: " << out << std::endl;

    // 1.1) 流水线支持拷贝/移动构造，可安全按值传递。
    // 这对“工厂函数返回 pipeline”或“将 pipeline 放入更高层对象”很有帮助。
    auto copied_pipeline = pipeline;
    auto moved_pipeline = std::move(copied_pipeline);
    const std::string out_moved = moved_pipeline.run(10);
    std::cout << "moved_pipeline output: " << out_moved << std::endl;

    // 2) 右值链式 builder：仅支持临时对象链式追加，避免左值误用拷贝路径。
    // 若阶段是条件编译或模板推导产物，builder 写法通常比一次性传参更易读。
    auto pipeline_builder = make_pipeline_builder<PipeForwardOrder::ordered>(8, 64)
                                .add_pipe(AddOnePipe{})
                                .add_pipe(ToPairPipe{})
                                .add_pipe(JoinPipe{})
                                .build();

    const std::string out_builder = pipeline_builder.run(10);
    std::cout << "pipeline_builder output: " << out_builder << std::endl;

    // 3) 无序模式示例。
    // 当业务更关心吞吐/尾延时而不要求“按输入顺序放行”时，可考虑 unordered。
    auto pipeline2 = make_pipeline<PipeForwardOrder::unordered>(
        8,
        64,
        PolicyPipe<PipeExecutionPolicy::shared_pool>{},
        PolicyPipe<PipeExecutionPolicy::dedicated_pool>{});

    const int out2 = pipeline2.run(3);
    std::cout << "pipeline2 output: " << out2 << std::endl;

    // 以下写法会在编译期报错（左值 builder 已禁用 add_pipe/build）：
    // auto b = make_pipeline_builder<PipeForwardOrder::ordered>(8, 64);
    // auto bad = b.add_pipe(AddOnePipe{}).build();

    return 0;
}