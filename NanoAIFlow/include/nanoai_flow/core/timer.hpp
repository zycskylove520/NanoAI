// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: timer.hpp
// Brief: 轻量栈式计时器，支持嵌套计时场景。
//
// 该计时器是开发辅助工具：
// - 通过栈结构支持嵌套 tic/toc；
// - 返回毫秒值并可选打印，便于快速 profiling。

#pragma once

#include <chrono>
#include <cstdio>
#include <stack>
#include <string>

namespace NanoAI_FLOW
{

/**
 * @brief 简易 tic/toc 计时器，通过栈支持嵌套计时。
 *
 * 线程安全：
 * - 该类型不做内部同步，预期在单线程调试/基准脚本中作为局部变量使用。
 * 生命周期：
 * - 纯栈式工具类，不持有外部资源；reset()/析构后不会有后台行为残留。
 */
class Timer
{
public:
    /// 栈顶元素表示当前计时区间的起点时间；多层嵌套时后进先出对应最内层区间。
    std::stack<std::chrono::high_resolution_clock::time_point> tictoc_stack;

    /// 压入新的起点时间。
    void tic()
    {
        tictoc_stack.push(std::chrono::high_resolution_clock::now());
    }

    /**
     * @brief 结束当前计时区间，并按需打印耗时。
     * @param msg 输出信息前缀。
     * @param flag 为 true 且 msg 非空时打印耗时。
     * @return 当前 tic/toc 对应的耗时（毫秒）。
     *
     * 边界行为：当未先调用 tic() 时，toc() 返回 0.0，不抛异常。
     */
    double toc(std::string msg = "", bool flag = true)
    {
        if (tictoc_stack.empty())
        {
            return 0.0;
        }

        const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::high_resolution_clock::now() - tictoc_stack.top())
                              .count();

        if (!msg.empty() && flag)
        {
            std::printf("%s time elapsed: %lld ms\n", msg.c_str(), static_cast<long long>(diff));
        }

        tictoc_stack.pop();
        return static_cast<double>(diff);
    }

    /// 清空所有计时起点。
    /// 用途：当调用方提前放弃某段 profiling 流程时，可一次性丢弃未配对的 tic 状态。
    void reset()
    {
        tictoc_stack = std::stack<std::chrono::high_resolution_clock::time_point>();
    }
};

} // namespace NanoAI_FLOW
