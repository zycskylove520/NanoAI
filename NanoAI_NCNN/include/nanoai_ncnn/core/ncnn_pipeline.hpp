// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: ncnn_pipeline.hpp
// Brief: TODO - add file summary.
//

#pragma once

#include <string>
#include <type_traits>
#include <utility>

#include "nanoai_flow/core/pipeline.hpp"

namespace NanoAI_NCNN
{
    using NanoAI_FLOW::NanoPipeLine;
    using NanoAI_FLOW::nanoai_u32;

    template <typename... Pipes>
    class NcnnPipeLine : public NanoPipeLine<Pipes...>
    {
    public:
        explicit NcnnPipeLine(
            std::string name,
            nanoai_u32 shared_pool_size = 0,
            nanoai_u32 global_task_quota = 0,
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
    NcnnPipeLine(std::string, nanoai_u32, nanoai_u32, Pipes...) -> NcnnPipeLine<std::decay_t<Pipes>...>;

} // namespace NanoAI_NCNN
