// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: demo_main.cpp
// Brief: 展示 RKNN 人脸特征模型从加载、预处理到推理输出的最小完整 pipeline 用法。
//
// Design notes:
// - 示例优先演示 API 串联方式，不追求覆盖所有配置分支。
// - 使用 host 内存模式以降低示例环境准备门槛，便于快速验证基本链路。
//

#include <iostream>

#include <any>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <opencv2/opencv.hpp>

#include "nanoai_flow/core/pipeline.hpp"
#include "nanoai_rknn/core/rknn_runtime.hpp"
#include "nanoai_rknn/cv/rknn_cv_preprocess_pipe.hpp"
#include "nanoai_rknn/cv/rknn_cv_types.hpp"
#include "nanoai_rknn/cv/rknn_cv_infer_pipe.hpp"
#include "nanoai_rknn/cv/rknn_cv_load_pipe.hpp"
#include "nanoai_rknn/cv/rknn_cv_postprocess_pipe.hpp"


int main(int argc, char **argv)
{
    std::cout << "[NanoAI RKNN Minimal Demo]" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  nanoai_rknn_demo <face_rec_model.rknn> <aligned_face_rgb_image>" << std::endl;

    if (argc < 3)
    {
        std::cout << "Missing arguments, exit with usage only." << std::endl;
        return 0;
    }

    const std::string model_path = argv[1];
    const std::string image_path = argv[2];

    try
    {
        cv::Mat img = cv::imread(image_path, cv::IMREAD_COLOR);
        if (img.empty())
        {
            throw std::runtime_error("Failed to load image: " + image_path);
        }

        // 1) 直接构造四个 pipe。load pipe 负责惰性加载，其余 stage 只关注各自数据变换职责。
        auto ai_ctx = std::make_shared<NanoAI_RKNN::Helper::RKNNAIContext>();
        NanoAI_RKNN::CV::RknnLoadPipe<NanoAI_RKNN::RknnMemoryMode::host> load_pipe(model_path, ai_ctx);
        NanoAI_RKNN::CV::RknnCvPreprocessPipe_RgbNormalize<> preprocess_pipe(true);
        NanoAI_RKNN::CV::RknnHostInferPipe<> infer_pipe;
        NanoAI_RKNN::CV::RknnHostPostprocessPipe<std::vector<float>> post_pipe(
            [](const NanoAI_RKNN::CV::RknnCvInferResult &result) -> std::vector<float> {
                if (result.outputs.empty())
                {
                    return std::vector<float>{};
                }
                return result.outputs.front().data;
            });

        // 2) 使用 builder API 固化执行顺序。ordered 模式适合需要稳定输入输出配对的推理任务。
        auto face_recognition_pipeline = NanoAI_FLOW::make_pipeline_builder<NanoAI_FLOW::PipeForwardOrder::ordered>(8, 64)
                    .add_pipe(load_pipe)
                    .add_pipe(preprocess_pipe)
                    .add_pipe(infer_pipe)
                    .add_pipe(post_pipe)
                    .build();

        // 3) 运行一次完整推理。示例仅打印向量维度和前几个值，方便快速确认模型确实产出结果。
        NanoAI_RKNN::CV::RknnPipelineInput input_data;
        input_data.image = img;

        std::vector<float> feature = face_recognition_pipeline.run(input_data);

        std::cout << "feature dim = " << feature.size() << std::endl;
        if (!feature.empty())
        {
            std::cout << "feature[0..3] = "
                      << feature[0] << ", "
                      << (feature.size() > 1 ? feature[1] : 0.0F) << ", "
                      << (feature.size() > 2 ? feature[2] : 0.0F) << ", "
                      << (feature.size() > 3 ? feature[3] : 0.0F)
                      << std::endl;
        }

        // 示例中手动释放 RKNN 资源，避免读者误以为 shared_ptr 会自动处理底层 C API 句柄。
        if (ai_ctx)
        {
            NanoAI_RKNN::Helper::ReleaseAIContext(*ai_ctx);
        }
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[NanoAI RKNN Minimal Demo] exception: " << e.what() << std::endl;
        return 1;
    }
}
