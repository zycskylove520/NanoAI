// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: types.h
// Brief: 基于 C++20 的跨平台基础类型声明（NanoAI 命名风格）。

#pragma once

#include <cstddef>
#include <cstdint>

namespace NanoAI_FLOW
{

    // 基础布尔与字符类型
    using nanoai_bool = bool;
    using nanoai_char = char;
    using nanoai_schar = signed char;
    using nanoai_uchar = unsigned char;
    using nanoai_wchar = wchar_t;
    using nanoai_char8 = unsigned char;
    using nanoai_char16 = char16_t;
    using nanoai_char32 = char32_t;

    // 固定宽度整数（推荐优先使用）
    using nanoai_i8 = std::int8_t;
    using nanoai_i16 = std::int16_t;
    using nanoai_i32 = std::int32_t;
    using nanoai_i64 = std::int64_t;

    using nanoai_u8 = std::uint8_t;
    using nanoai_u16 = std::uint16_t;
    using nanoai_u32 = std::uint32_t;
    using nanoai_u64 = std::uint64_t;

    // 最小宽度整数
    using nanoai_i_least8 = std::int_least8_t;
    using nanoai_i_least16 = std::int_least16_t;
    using nanoai_i_least32 = std::int_least32_t;
    using nanoai_i_least64 = std::int_least64_t;

    using nanoai_u_least8 = std::uint_least8_t;
    using nanoai_u_least16 = std::uint_least16_t;
    using nanoai_u_least32 = std::uint_least32_t;
    using nanoai_u_least64 = std::uint_least64_t;

    // 快速整数
    using nanoai_i_fast8 = std::int_fast8_t;
    using nanoai_i_fast16 = std::int_fast16_t;
    using nanoai_i_fast32 = std::int_fast32_t;
    using nanoai_i_fast64 = std::int_fast64_t;

    using nanoai_u_fast8 = std::uint_fast8_t;
    using nanoai_u_fast16 = std::uint_fast16_t;
    using nanoai_u_fast32 = std::uint_fast32_t;
    using nanoai_u_fast64 = std::uint_fast64_t;

    // 最大宽度整数
    using nanoai_imax = std::intmax_t;
    using nanoai_umax = std::uintmax_t;

    // 平台相关宽度
    using nanoai_usize = std::size_t;
    using nanoai_isize = std::ptrdiff_t;
    using nanoai_uptr = std::uintptr_t;
    using nanoai_iptr = std::intptr_t;

    // 浮点
    using nanoai_f32 = float;
    using nanoai_f64 = double;
    using nanoai_f80 = long double;

    // 字节语义
    using nanoai_byte = std::byte;

} // namespace NanoAI_FLOW
