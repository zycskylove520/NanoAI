#pragma once

#include <any>
#include <memory>
#include <string>
#include <vector>

#include <ncnn/net.h>
#include <ncnn/mat.h>

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

    struct NcnnCvMeta
    {
        float ratio{1.0F};
        int pad_x{0};
        int pad_y{0};
        int src_width{0};
        int src_height{0};
        int dst_width{0};
        int dst_height{0};
    };

    struct NcnnInferResult
    {
        std::shared_ptr<NcnnRuntimeContext> runtime{};
        ncnn::Mat output{};
        NcnnCvMeta meta{};
        std::any user_data{};
    };

} // namespace NanoAI_NCNN
