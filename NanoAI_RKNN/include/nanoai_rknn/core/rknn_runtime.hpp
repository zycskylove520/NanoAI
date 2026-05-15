// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: rknn_runtime.hpp
// Brief: 为 RKNN 运行时加载流程提供惰性封装，统一模型加载、初始化与零拷贝资源准备。
//
// Design notes:
// - 该文件面向 pipeline 首段的加载阶段，避免每次推理重复初始化 RKNN 运行时。
// - 当前实现未做内部同步，要求同一个 loader 在外部按单线程初始化或只在首次运行前串行触发。
//

#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "rknn_types.hpp"

namespace NanoAI_RKNN
{
    // after_load 回调在基础 RKNN 资源准备完成后执行，适合补充业务侧定制初始化。
    using RknnAfterLoadCallback = std::function<int(Helper::RKNNAIContext &)>;

    template <RknnMemoryMode Mode>
    // RknnRuntimeLoader 管理单个模型实例的惰性加载状态。
    // 线程安全：不提供内部同步；并发首次加载需要外部串行化。
    // 生命周期：可移动/可拷贝依赖成员默认语义，但通常建议按单实例长期复用。
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

        // 返回共享运行时上下文，供下游 stage 复用 RKNN 句柄与张量属性。
        const std::shared_ptr<Helper::RKNNAIContext> &context() const noexcept
        {
            return runtime_.ai_ctx;
        }

        // 更新模型路径会使已加载状态失效，下一次 ensure_loaded 会重新装载模型。
        void set_model_path(std::string model_path)
        {
            runtime_.spec.model_path = std::move(model_path);
            loaded_ = false;
        }

        // 整体替换模型规格时强制回写模板指定的 memory mode，避免外部传入不一致配置。
        void set_model_spec(RknnModelSpec spec)
        {
            runtime_.spec = std::move(spec);
            runtime_.spec.memory_mode = Mode;
            loaded_ = false;
        }

        // 仅重置逻辑加载标记，不主动释放已有 RKNN 资源；资源回收由上下文持有方负责。
        void reset_loaded() noexcept
        {
            loaded_ = false;
        }

        // 保证模型至少成功加载一次。
        // 异常：任何底层 RKNN 初始化失败都会抛出 std::runtime_error，方便 pipeline 直接终止。
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

            // 先加载模型文件，再查询 IO 属性；后续预处理/推理阶段依赖这些元数据决定张量布局。
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
                // zero-copy 模式必须在首次加载时提前分配并绑定 IO 内存，后续推理阶段才可直接 memcpy 到共享缓冲区。
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
                // 自定义回调用于衔接额外业务初始化，例如 DMA、RGA 或模型后处理元数据缓存。
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
        // loaded_ 仅表示本 loader 已完成初始化流程，不表示 ai_ctx 生命周期归当前对象所有。
        bool loaded_{false};
    };

} // namespace NanoAI_RKNN
