// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: copy.hpp
// Brief: 提供禁止拷贝/移动语义的工具基类。

#pragma once

namespace NanoAI_FLOW
{
    /**
        * @brief 禁止拷贝的混入基类。
     */
    class NoCopyable
    {
    protected:
        NoCopyable() = default;
        ~NoCopyable() = default;

        NoCopyable(const NoCopyable &) = delete;
        NoCopyable &operator=(const NoCopyable &) = delete;
    };

    /**
        * @brief 禁止移动的混入基类。
     */
    class NoMoveable
    {
    protected:
        NoMoveable() = default;
        ~NoMoveable() = default;

        NoMoveable(NoMoveable &&) = delete;
        NoMoveable &operator=(NoMoveable &&) = delete;
    };

    /**
        * @brief 同时禁止拷贝与移动的混入基类。
     */
    class NoCopyMoveable
    {
    protected:
        NoCopyMoveable() = default;
        ~NoCopyMoveable() = default;

        NoCopyMoveable(const NoCopyMoveable &) = delete;
        NoCopyMoveable &operator=(const NoCopyMoveable &) = delete;

        NoCopyMoveable(NoCopyMoveable &&) = delete;
        NoCopyMoveable &operator=(NoCopyMoveable &&) = delete;
    };

} // namespace NanoAI_FLOW
