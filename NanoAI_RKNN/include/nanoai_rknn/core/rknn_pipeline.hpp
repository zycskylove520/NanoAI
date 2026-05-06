#pragma once

#include <string>
#include <type_traits>
#include <utility>

#include "nanoai_flow/core/pipeline.hpp"

namespace NanoAI_RKNN
{
    using NanoAI_FLOW::NanoPipeLine;
    using NanoAI_FLOW::PipeCount;

    template <typename... Pipes>
    class RknnPipeLine : public NanoPipeLine<Pipes...>
    {
    public:
        explicit RknnPipeLine(
            std::string name,
            PipeCount shared_pool_size = 0,
            PipeCount global_task_quota = 0,
            Pipes... pipes)
            : NanoPipeLine<Pipes...>(shared_pool_size, global_task_quota, std::forward<Pipes>(pipes)...),
              name_(std::move(name))
        {
        }

        const std::string &name() const noexcept
        {
            return name_;
        }

        void set_name(std::string name)
        {
            name_ = std::move(name);
        }

    private:
        std::string name_;
    };

    template <typename... Pipes>
    RknnPipeLine(std::string, PipeCount, PipeCount, Pipes...) -> RknnPipeLine<std::decay_t<Pipes>...>;

} // namespace NanoAI_RKNN
