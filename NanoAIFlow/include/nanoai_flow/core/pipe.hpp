#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

namespace NanoAI_FLOW
{

enum class PipeExecutionPolicy
{
    shared_pool,
    dedicated_pool,
    inline_run
};

using PipeCount = std::uint32_t;

template <typename T>
struct is_reference_wrapper : std::false_type
{
};

template <typename U>
struct is_reference_wrapper<std::reference_wrapper<U>> : std::true_type
{
};

template <typename T>
struct unwrap_reference_wrapper
{
    using type = std::remove_cvref_t<T>;
};

template <typename U>
struct unwrap_reference_wrapper<std::reference_wrapper<U>>
{
    using type = U;
};

template <typename T>
using unwrap_reference_wrapper_t = typename unwrap_reference_wrapper<std::remove_cvref_t<T>>::type;

template <typename T>
concept PipeStage = requires(unwrap_reference_wrapper_t<T> &pipe)
{
    { pipe.concurrency() } -> std::convertible_to<PipeCount>;
    { pipe.execution_policy() } -> std::same_as<PipeExecutionPolicy>;
    { pipe.dedicated_pool_size() } -> std::convertible_to<PipeCount>;
};

template <
    typename Derived,
    PipeCount MaxConcurrency = 1,
    PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
    PipeCount DedicatedPoolSize = 0>
class NanoPipe
{
    static constexpr PipeCount normalized_concurrency = MaxConcurrency == 0 ? 1 : MaxConcurrency;

public:
    constexpr NanoPipe() noexcept = default;
    ~NanoPipe() = default;

    constexpr PipeCount concurrency() const noexcept
    {
        return normalized_concurrency;
    }

    constexpr PipeExecutionPolicy execution_policy() const noexcept
    {
        return Policy;
    }

    constexpr PipeCount dedicated_pool_size() const noexcept
    {
        if constexpr (Policy == PipeExecutionPolicy::dedicated_pool)
        {
            if constexpr (DedicatedPoolSize > 0)
            {
                return DedicatedPoolSize;
            }
            return normalized_concurrency;
        }
        return 0;
    }

    template <typename... Args>
    decltype(auto) run(Args &&...args)
    {
        return static_cast<Derived *>(this)->on_run(std::forward<Args>(args)...);
    }

    template <typename... Args>
    decltype(auto) run(Args &&...args) const
    {
        return static_cast<const Derived *>(this)->on_run(std::forward<Args>(args)...);
    }
};

} // namespace NanoAI_FLOW
