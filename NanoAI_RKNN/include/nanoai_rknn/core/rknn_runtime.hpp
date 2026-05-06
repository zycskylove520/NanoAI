#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "rknn_types.hpp"

namespace NanoAI_RKNN
{
    using RknnAfterLoadCallback = std::function<int(Helper::RKNNAIContext &)>;

    template <RknnMemoryMode Mode>
    class RknnRuntimeLoader
    {
    public:
        explicit RknnRuntimeLoader(
            RknnModelSpec spec,
            std::shared_ptr<Helper::RKNNAIContext> ai_ctx = nullptr,
            RknnAfterLoadCallback after_load = nullptr) noexcept
            : runtime_{std::move(ai_ctx), std::move(spec)},
              after_load_(std::move(after_load))
        {
            runtime_.spec.memory_mode = Mode;
        }

        const RknnRuntimeContext &runtime() const noexcept
        {
            return runtime_;
        }

        const std::shared_ptr<Helper::RKNNAIContext> &context() const noexcept
        {
            return runtime_.ai_ctx;
        }

        void set_model_path(std::string model_path)
        {
            runtime_.spec.model_path = std::move(model_path);
            loaded_ = false;
        }

        void set_model_spec(RknnModelSpec spec)
        {
            runtime_.spec = std::move(spec);
            runtime_.spec.memory_mode = Mode;
            loaded_ = false;
        }

        void reset_loaded() noexcept
        {
            loaded_ = false;
        }

        void ensure_loaded()
        {
            if (loaded_)
            {
                return;
            }

            if (runtime_.spec.model_path.empty())
            {
                throw std::invalid_argument("RknnRuntimeLoader::ensure_loaded: model_path is empty");
            }

            if (!runtime_.ai_ctx)
            {
                runtime_.ai_ctx = std::make_shared<Helper::RKNNAIContext>();
            }

            int ret = Helper::LoadModel(*runtime_.ai_ctx, runtime_.spec.model_path);
            if (ret != 0)
            {
                throw std::runtime_error("RknnRuntimeLoader::ensure_loaded: LoadModel failed, ret=" + std::to_string(ret));
            }

            ret = Helper::InitModel(*runtime_.ai_ctx);
            if (ret != 0)
            {
                throw std::runtime_error("RknnRuntimeLoader::ensure_loaded: InitModel failed, ret=" + std::to_string(ret));
            }

            if constexpr (Mode == RknnMemoryMode::zero_copy)
            {
                ret = Helper::CreateZeroCopyInputTensor(*runtime_.ai_ctx);
                if (ret != 0)
                {
                    throw std::runtime_error("RknnRuntimeLoader::ensure_loaded: CreateZeroCopyInputTensor failed, ret=" + std::to_string(ret));
                }

                ret = Helper::CreateZeroCopyOutputTensor(*runtime_.ai_ctx);
                if (ret != 0)
                {
                    throw std::runtime_error("RknnRuntimeLoader::ensure_loaded: CreateZeroCopyOutputTensor failed, ret=" + std::to_string(ret));
                }
            }

            if (after_load_)
            {
                ret = after_load_(*runtime_.ai_ctx);
                if (ret != 0)
                {
                    throw std::runtime_error("RknnRuntimeLoader::ensure_loaded: after_load callback failed, ret=" + std::to_string(ret));
                }
            }

            loaded_ = true;
        }

    private:
        RknnRuntimeContext runtime_{};
        RknnAfterLoadCallback after_load_{};
        bool loaded_{false};
    };

} // namespace NanoAI_RKNN
