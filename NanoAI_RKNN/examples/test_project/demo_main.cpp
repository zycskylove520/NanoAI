// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: demo_main.cpp
// Brief: TODO - add file summary.
//

#include <iostream>

#include <any>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <opencv2/opencv.hpp>

#include "nanoai_flow/core/pipeline.hpp"
#include "nanoai_rknn/core/rknn_runtime.hpp"
#include "nanoai_rknn/core/rknn_types.hpp"
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

        // 1) 直接构造四个 pipe
        auto ai_ctx = std::make_shared<NanoAI_RKNN::Helper::RKNNAIContext>();
        NanoAI_RKNN::CV::RknnHostLoadPipe<> load_pipe(model_path, ai_ctx);
        NanoAI_RKNN::CV::RknnHostRgbToFp16Pipe<> preprocess_pipe;
        NanoAI_RKNN::CV::RknnHostInferPipe<> infer_pipe;
        NanoAI_RKNN::CV::RknnHostPostprocessPipe<std::vector<float>> post_pipe(
            [](const NanoAI_RKNN::RknnInferResult &result) -> std::vector<float> {
                if (result.outputs.empty())
                {
                    return std::vector<float>{};
                }
                return result.outputs.front().data;
            });

        // 2) add_pipe 串起来（add_pipe 返回新管线对象）
        auto face_recognition_pipeline = NanoAI_FLOW::NanoPipeLine(8, 64)
                            .add_pipe(load_pipe)
                            .add_pipe(preprocess_pipe)
                            .add_pipe(infer_pipe)
                            .add_pipe(post_pipe);

        // 3) run(input_data)
        NanoAI_RKNN::CV::RknnPipelineInput input_data;
        input_data.data = img.data;
        input_data.width = img.cols;
        input_data.height = img.rows;
        input_data.channels = 3;

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
