#pragma once

#include <string>
#include <type_traits>
#include <utility>

#include "nanoai_flow/core/pipeline.hpp"

namespace NanoAI_NCNN
{
    using NanoAI_FLOW::NanoPipeLine;
    using NanoAI_FLOW::PipeCount;

    template <typename... Pipes>
    class NcnnPipeLine : public NanoPipeLine<Pipes...>
    {
    public:
        explicit NcnnPipeLine(
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
    NcnnPipeLine(std::string, PipeCount, PipeCount, Pipes...) -> NcnnPipeLine<std::decay_t<Pipes>...>;

} // namespace NanoAI_NCNN
