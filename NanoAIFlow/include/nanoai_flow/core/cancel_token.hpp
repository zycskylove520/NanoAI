// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: cancel_token.hpp
// Brief: 协作式取消信号类型定义。
//
// 设计目标：
// - 提供线程安全的协作式取消信号对象。
// - 支持 pipeline/pipe/on_run 传递与检测。
// - 可扩展更多取消上下文信息。

#pragma once

#include <atomic>
#include <memory>

namespace NanoAI_FLOW
{

/**
 * @brief 协作式取消信号。
 *
 * 线程安全：
 * - cancelled 使用原子布尔，支持多线程并发 request/check。
 * 生命周期：
 * - 通过 shared_ptr 共享状态，拷贝 token 后仍指向同一取消源。
 */
struct NanoCancelToken
{
    // 共享取消状态；多个 token 副本指向同一原子位，便于跨 stage/跨线程传播取消信号。
    std::shared_ptr<std::atomic<bool>> cancelled;

    // 创建一个“未取消”状态的 token。
    NanoCancelToken() : cancelled(std::make_shared<std::atomic<bool>>(false)) {}

    // 发出取消请求。
    // 行为：仅置位，不阻塞等待执行线程退出（协作式语义）。
    void request_cancel() const
    {
        if (cancelled)
        {
            cancelled->store(true, std::memory_order_relaxed);
        }
    }

    // 查询当前是否已请求取消。
    // 返回：true 表示调用方应尽快走收敛/退出路径。
    bool is_cancelled() const
    {
        return cancelled && cancelled->load(std::memory_order_relaxed);
    }
};

/**
 * @brief 协作式取消上下文（预留扩展）。
 *
 * 用途：后续可在不破坏 NanoCancelToken 用法的前提下扩展取消原因、
 * 截止时间等上下文字段。
 */
struct NanoCancelContext
{
    // 当前上下文默认仅携带 token，保留结构体是为了将来扩展原因、deadline 等字段时不破坏 API 形状。
    NanoCancelToken token;
    // 可扩展更多上下文信息
};

} // namespace NanoAI_FLOW
