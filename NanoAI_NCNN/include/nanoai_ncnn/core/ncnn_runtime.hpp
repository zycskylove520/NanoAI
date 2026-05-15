// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: ncnn_runtime.hpp
// Brief: 封装 NCNN 模型的惰性加载流程，统一 `param/bin` 文件装载与运行时状态复用。
//
// Design notes:
// - 该文件服务于 pipeline 首段加载逻辑，避免每次推理重复构造与初始化 `ncnn::Net`。
// - 当前实现不提供内部锁，约定首次加载或重载行为由外部串行触发。
//

#pragma once

#include <stdexcept>
#include <string>
#include <utility>

#include "nanoai_ncnn/core/ncnn_types.hpp"

namespace NanoAI_NCNN
{
    // NcnnRuntimeLoader 管理单个 NCNN 模型实例的加载状态与共享运行时上下文。
    // 线程安全：首次 ensure_loaded / set_model_spec / reset_loaded 需外部同步。
    // 生命周期：通过 shared_ptr 共享 runtime，便于多个 pipe 共同复用同一 `ncnn::Net`。
    class NcnnRuntimeLoader
    {
    public:
        explicit NcnnRuntimeLoader(NcnnModelSpec spec, std::shared_ptr<NcnnRuntimeContext> runtime = nullptr)
            : runtime_(runtime ? std::move(runtime) : std::make_shared<NcnnRuntimeContext>())
        {
            runtime_->spec = std::move(spec);
            runtime_->loaded = false;
        }

        // 确保模型至少完成一次成功加载。
        // 异常：路径缺失或 NCNN 装载失败时抛出标准异常，方便 pipeline 直接中止。
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
                // 重载同一 runtime 时先 clear，避免旧模型层定义与新模型混用。
                runtime_->net->clear();
            }
            // 线程数与 Vulkan 选项在 load_param/load_model 前写入，确保后续 extractor 继承正确配置。
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

        // 返回共享 runtime，供下游 stage 创建 extractor 或读取最终生效配置。
        const std::shared_ptr<NcnnRuntimeContext> &runtime() const noexcept
        {
            return runtime_;
        }

        // 替换模型规格时主动丢弃旧 `ncnn::Net`，避免新旧模型文件交叉污染。
        void set_model_spec(NcnnModelSpec spec)
        {
            runtime_->spec = std::move(spec);
            runtime_->loaded = false;
            runtime_->net.reset();
        }

        // 强制下次重新加载，同时释放当前 `ncnn::Net` 占用的模型资源。
        void reset_loaded()
        {
            runtime_->loaded = false;
            runtime_->net.reset();
        }

    private:
        std::shared_ptr<NcnnRuntimeContext> runtime_;
    };

} // namespace NanoAI_NCNN
