#pragma once

#include <chrono>
#include <cstdio>
#include <stack>
#include <string>

namespace NanoAI_FLOW
{

class Timer
{
public:
    std::stack<std::chrono::high_resolution_clock::time_point> tictoc_stack;

    void tic()
    {
        tictoc_stack.push(std::chrono::high_resolution_clock::now());
    }

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

    void reset()
    {
        tictoc_stack = std::stack<std::chrono::high_resolution_clock::time_point>();
    }
};

} // namespace NanoAI_FLOW
