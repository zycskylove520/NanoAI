// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: defines.hpp
// Brief: 常用编译期工具与参数访问辅助函数。
//
// 注意：此文件主要服务于模板元编程与参数包工具，
// 用于减少 pipeline/pipe 代码中的重复样板。

#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace NanoAI_FLOW
{

/// 调试开关：1 开启调试输出与计时，0 关闭。
/// 注意：该宏为编译期开关，不是线程安全的运行时日志配置入口。
#define NANOAI_DEBUG 1

/**
 * @brief 从转发参数包中按编译期索引提取参数。
 *
 * 输入：args 为任意转发参数包，Index 为编译期常量下标。
 * 返回：Index 对应参数，并保留原始值类别（左值/右值）。
 *
 * 保留原始值类别：
 * - 若参数为左值，返回左值引用；
 * - 若参数为右值，返回右值引用。
 */
template <int Index, typename... Args>
decltype(auto) get_args_element(Args &&...args)
{
    return std::get<Index>(
        std::forward_as_tuple(std::forward<Args>(args)...));
}

/**
 * @brief get_args_element 的编译期类型检查辅助。
 *
 * 使用方式：在模板上下文中显式调用 check<T, Index>(args...)。
 * 异常/错误传播：不抛异常，类型不匹配时在编译期 static_assert 失败。
 */
struct check_element_type
{
    template <typename T, int Index, typename... Args>
    static void check(Args &&...)
    {
        using ElementType = std::tuple_element_t<Index, std::tuple<std::remove_reference_t<Args>...>>;
        static_assert(std::is_same_v<T, ElementType>, "get_args_element type mismatch at index");
    }
};

} // namespace NanoAI_FLOW
