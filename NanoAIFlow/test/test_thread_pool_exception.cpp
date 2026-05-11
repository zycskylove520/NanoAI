// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: test_thread_pool_exception.cpp
// Brief: 验证线程池任务异常兜底、异常回调与异常计数行为。

#include <nanoai_flow/core/thread_pool.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <semaphore>
#include <stdexcept>

using namespace NanoAI_FLOW;

int main()
{
    NanoThreadPool pool(2);

    std::atomic<int> handler_calls{0};
    std::binary_semaphore handler_sem{0};

    pool.set_exception_handler([&](std::exception_ptr ep) {
        ++handler_calls;
        try
        {
            if (ep)
            {
                std::rethrow_exception(ep);
            }
        }
        catch (const std::runtime_error &)
        {
            // 预期异常类型。
        }
        catch (...)
        {
            // 其他异常类型也视为已捕获。
        }
        handler_sem.release();
    });

    pool.detach_task([] {
        throw std::runtime_error("expected test exception");
    });

    const bool got_handler = handler_sem.try_acquire_for(std::chrono::seconds(2));
    const nanoai_u64 ex_count = pool.task_exception_count();

    std::cout << "[RESULT] thread_pool_exception"
              << " got_handler=" << (got_handler ? "YES" : "NO")
              << " handler_calls=" << handler_calls.load(std::memory_order_relaxed)
              << " ex_count=" << ex_count
              << '\n';

    // 防回归：异常必须同时体现为“回调被触发”与“异常计数增加”。
    if (!got_handler || handler_calls.load(std::memory_order_relaxed) <= 0 || ex_count <= 0)
    {
        std::cout << "[TEST] thread_pool_exception=FAIL\n";
        return 1;
    }

    pool.reset_task_exception_count();
    // 防回归：reset 后计数必须清零，避免历史异常污染后续观测。
    const bool reset_ok = (pool.task_exception_count() == 0);
    std::cout << "[RESULT] thread_pool_exception_reset"
              << " reset_ok=" << (reset_ok ? "YES" : "NO")
              << '\n';

    std::cout << "[TEST] thread_pool_exception=" << ((reset_ok) ? "PASS" : "FAIL") << '\n';
    return reset_ok ? 0 : 1;
}
