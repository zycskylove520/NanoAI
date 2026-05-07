// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: defines.hpp
// Brief: 常用编译期工具与参数访问辅助函数。

#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace NanoAI_FLOW
{

/// 调试开关：1 开启调试输出与计时，0 关闭。
#define NANOAI_DEBUG 1

/**
 * @brief 从转发参数包中按编译期索引提取参数。
 */
template <int Index, typename... Args>
decltype(auto) get_args_element(Args &&...args)
{
    return std::get<Index>(
        std::forward_as_tuple(std::forward<Args>(args)...));
}

/**
 * @brief get_args_element 的编译期类型检查辅助。
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
