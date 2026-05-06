#pragma once

#include <stdexcept>
#include <string>
#include <utility>

#include "nanoai_ncnn/core/ncnn_types.hpp"

namespace NanoAI_NCNN
{
    class NcnnRuntimeLoader
    {
    public:
        explicit NcnnRuntimeLoader(NcnnModelSpec spec, std::shared_ptr<NcnnRuntimeContext> runtime = nullptr)
            : runtime_(runtime ? std::move(runtime) : std::make_shared<NcnnRuntimeContext>())
        {
            runtime_->spec = std::move(spec);
            runtime_->loaded = false;
        }

        void ensure_loaded()
        {
            if (runtime_->loaded)
            {
                return;
            }

            if (runtime_->spec.param_path.empty() || runtime_->spec.bin_path.empty())
            {
                throw std::invalid_argument("NcnnRuntimeLoader::ensure_loaded: model paths are empty");
            }

            if (!runtime_->net)
            {
                runtime_->net = std::make_shared<ncnn::Net>();
            }
            else
            {
                runtime_->net->clear();
            }
            runtime_->net->opt.num_threads = runtime_->spec.num_threads;
            runtime_->net->opt.use_vulkan_compute = runtime_->spec.use_vulkan;

            const int param_ret = runtime_->net->load_param(runtime_->spec.param_path.c_str());
            if (param_ret != 0)
            {
                throw std::runtime_error("NcnnRuntimeLoader::ensure_loaded: load_param failed: " + runtime_->spec.param_path);
            }

            const int bin_ret = runtime_->net->load_model(runtime_->spec.bin_path.c_str());
            if (bin_ret != 0)
            {
                throw std::runtime_error("NcnnRuntimeLoader::ensure_loaded: load_model failed: " + runtime_->spec.bin_path);
            }

            runtime_->loaded = true;
        }

        const std::shared_ptr<NcnnRuntimeContext> &runtime() const noexcept
        {
            return runtime_;
        }

        void set_model_spec(NcnnModelSpec spec)
        {
            runtime_->spec = std::move(spec);
            runtime_->loaded = false;
            runtime_->net.reset();
        }

        void reset_loaded()
        {
            runtime_->loaded = false;
            runtime_->net.reset();
        }

    private:
        std::shared_ptr<NcnnRuntimeContext> runtime_;
    };

} // namespace NanoAI_NCNN
