// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: copy.hpp
// Brief: 提供禁止拷贝/移动语义的工具基类。
//
// 说明：
// - 这些类型用于显式表达“资源所有权不可复制/不可移动”的设计意图；
// - 相比散落在各类中的 delete 声明，继承式写法更集中、可复用。

#pragma once

namespace NanoAI_FLOW
{
/**
 * @brief 禁止拷贝的混入基类。
 *
 * 典型用途：对象可移动但不允许复制（如持有互斥量、文件句柄等资源）。
 */
class NoCopyable
{
protected:
    NoCopyable() = default;
    // 保护析构可以避免通过基类指针误删，也强调这些类型只作为 mixin 使用。
    ~NoCopyable() = default;

    NoCopyable(const NoCopyable &) = delete;
    NoCopyable &operator=(const NoCopyable &) = delete;
};

/**
 * @brief 禁止移动的混入基类。
 *
 * 典型用途：对象地址必须稳定，但允许复制语义（较少见）。
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
 *
 * 典型用途：对象管理独占运行时资源，既不能复制也不能迁移。
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
