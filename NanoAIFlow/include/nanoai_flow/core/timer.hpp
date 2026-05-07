// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: timer.hpp
// Brief: 轻量栈式计时器，支持嵌套计时场景。

#pragma once

#include <chrono>
#include <cstdio>
#include <stack>
#include <string>

namespace NanoAI_FLOW
{

/**
 * @brief 简易 tic/toc 计时器，通过栈支持嵌套计时。
 */
class Timer
{
public:
    /// 栈顶元素表示当前计时区间的起点时间。
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
    void reset()
    {
        tictoc_stack = std::stack<std::chrono::high_resolution_clock::time_point>();
    }
};

} // namespace NanoAI_FLOW
