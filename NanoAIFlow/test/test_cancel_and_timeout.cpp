// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: test_cancel_and_timeout.cpp
// Brief: 测试协作式取消与超时中断能力。
//
// 本文件所有注释均遵循 comment_guidelines.md 要求。

#include "nanoai_flow/core/pipeline.hpp"
#include <thread>
#include <chrono>
#include <cassert>
#include <iostream>

using namespace NanoAI_FLOW;

/**
 * @brief 可取消的慢 pipe。
 * on_run 检查 cancel_token，支持协作式取消。
 */
class SlowCancellablePipe : public NanoPipe<SlowCancellablePipe>
{
public:
    int on_run(int x)
    {
        NanoCancelToken token;
        return on_run(x, token);
    }

    int on_run(int x, const NanoCancelToken& cancel_token)
    {
        for (int i = 0; i < 100; ++i) {
            if (cancel_token.is_cancelled())
                throw std::runtime_error("cancelled");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return x + 1;
    }
};

/**
 * @brief 不可取消的 pipe。
 */
class NonCancellablePipe : public NanoPipe<NonCancellablePipe>
{
public:
    int on_run(int x) { return x * 2; }
};

int main() {
    // 测试 1：run_with_cancel 正常完成
    {
        SlowCancellablePipe p;
        auto pipeline = make_pipeline<PipeForwardOrder::ordered>(NanoPipeLineOptions{}, p);
        NanoCancelToken token;
        int out = pipeline.run_with_cancel(token, 1);
        // 防回归：未取消路径必须返回正常业务结果。
        assert(out == 2);
    }
    // 测试 2：run_with_cancel 被主动取消
    {
        SlowCancellablePipe p;
        auto pipeline = make_pipeline<PipeForwardOrder::ordered>(NanoPipeLineOptions{}, p);
        NanoCancelToken token;
        std::thread t([&](){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            token.request_cancel();
        });
        try {
            pipeline.run_with_cancel(token, 1);
            // 防回归：若未抛异常，表示取消信号被忽略。
            assert(false && "should be cancelled");
        } catch (const std::exception& e) {
            // 只要进入异常分支即可，异常文本作为调试信息输出。
            std::cout << "cancelled as expected: " << e.what() << std::endl;
        }
        t.join();
    }
    // 测试 3：run_timeout_ms 超时
    {
        SlowCancellablePipe p;
        NanoPipeLineOptions opts;
        opts.run_timeout_ms = 20;
        auto pipeline = make_pipeline<PipeForwardOrder::ordered>(opts, p);
        try {
            pipeline.run(1);
            // 防回归：超时场景必须失败，避免 run_timeout_ms 失效。
            assert(false && "should timeout");
        } catch (const std::exception& e) {
            std::cout << "timeout as expected: " << e.what() << std::endl;
        }
    }
    // 测试 4：shutdown 后拒绝新任务
    {
        SlowCancellablePipe p;
        auto pipeline = make_pipeline<PipeForwardOrder::ordered>(NanoPipeLineOptions{}, p);
        pipeline.shutdown();
        try {
            pipeline.run(1);
            // 防回归：shutdown 后必须拒绝新任务。
            assert(false && "should be rejected after shutdown");
        } catch (const std::exception& e) {
            std::cout << "rejected after shutdown as expected: " << e.what() << std::endl;
        }
    }
    // 测试 5：不可取消 pipe 兼容
    {
        NonCancellablePipe p;
        auto pipeline = make_pipeline<PipeForwardOrder::ordered>(NanoPipeLineOptions{}, p);
        int out = pipeline.run(2);
        // 防回归：不支持取消参数的 pipe 仍需保持兼容执行。
        assert(out == 4);
    }
    std::cout << "All cancel/timeout tests passed." << std::endl;
    return 0;
}
