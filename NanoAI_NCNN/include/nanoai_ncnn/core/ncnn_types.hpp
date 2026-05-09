// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: ncnn_types.hpp
// Brief: TODO - add file summary.
//

#pragma once

#include <any>
#include <memory>
#include <string>
#include <vector>

#include <ncnn/net.h>
#include <ncnn/mat.h>

#include "nanoai_flow/core/types.h"

namespace NanoAI_NCNN
{
    enum class NcnnModelDomain : unsigned char
    {
        cv = 0,
        custom = 255,
    };

    struct NcnnModelSpec
    {
        std::string param_path;
        std::string bin_path;
        NcnnModelDomain domain{NcnnModelDomain::cv};
        int num_threads{2};
        bool use_vulkan{false};
    };

    struct NcnnRuntimeContext
    {
        std::shared_ptr<ncnn::Net> net{};
        NcnnModelSpec spec{};
        bool loaded{false};
    };

} // namespace NanoAI_NCNN
