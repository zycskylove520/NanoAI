#pragma once

#include <stdexcept>
#include <string>
#include <tuple>

#include "nanoai_flow/core/pipe.hpp"
#include "nanoai_ncnn/cv/ncnn_cv_types.hpp"

namespace NanoAI_NCNN::CV
{
    using NanoAI_FLOW::NanoPipe;
    using NanoAI_FLOW::nanoai_u32;
    using NanoAI_FLOW::PipeExecutionPolicy;

    template <
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class NcnnCvInferPipe : public NanoPipe<NcnnCvInferPipe<NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        explicit NcnnCvInferPipe(std::string input_name = "images", std::string output_name = "output0")
            : input_name_(std::move(input_name)),
              output_name_(std::move(output_name))
        {
        }

        auto on_run(const std::shared_ptr<NcnnRuntimeContext> &runtime, const NcnnPreprocessPacket &input)
        {
            if (!runtime || !runtime->net)
            {
                throw std::invalid_argument("NcnnCvInferPipe::on_run: runtime is null or not loaded");
            }

            ncnn::Extractor extractor = runtime->net->create_extractor();
            const int input_ret = extractor.input(input_name_.c_str(), input.input);
            if (input_ret != 0)
            {
                throw std::runtime_error("NcnnCvInferPipe::on_run: extractor.input failed");
            }

            NcnnInferResult result;
            result.runtime = runtime;
            result.meta = input.meta;
            result.user_data = input.user_data;

            const int extract_ret = extractor.extract(output_name_.c_str(), result.output);
            if (extract_ret != 0)
            {
                throw std::runtime_error("NcnnCvInferPipe::on_run: extractor.extract failed");
            }

            return std::make_tuple(runtime, std::move(result));
        }

    private:
        std::string input_name_;
        std::string output_name_;
    };

} // namespace NanoAI_NCNN::CV
