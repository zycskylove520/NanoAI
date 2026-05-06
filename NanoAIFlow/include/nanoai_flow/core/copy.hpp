#pragma once

namespace NanoAI_FLOW
{
    class NoCopyable
    {
    protected:
        NoCopyable() = default;
        ~NoCopyable() = default;

        NoCopyable(const NoCopyable &) = delete;
        NoCopyable &operator=(const NoCopyable &) = delete;
    };

    class NoMoveable
    {
    protected:
        NoMoveable() = default;
        ~NoMoveable() = default;

        NoMoveable(NoMoveable &&) = delete;
        NoMoveable &operator=(NoMoveable &&) = delete;
    };

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
