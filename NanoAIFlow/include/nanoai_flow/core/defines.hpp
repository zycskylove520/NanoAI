#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace NanoAI_FLOW
{

#define NANOAI_DEBUG 1 ///< Enable debug output and timing. 1=on, 0=off.

template <int Index, typename... Args>
decltype(auto) get_args_element(Args &&...args)
{
    return std::get<Index>(
        std::forward_as_tuple(std::forward<Args>(args)...));
}

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
