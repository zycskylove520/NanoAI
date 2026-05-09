// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: car_pipeline.hpp
// Brief: TODO - add file summary.
//

#pragma once

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "nanoai_flow/core/pipeline.hpp"
#include "nanoai_ncnn/cv/pipe/ncnn_cv_infer_pipe.hpp"
#include "nanoai_ncnn/cv/pipe/ncnn_cv_load_pipe.hpp"
#include "nanoai_ncnn/cv/pipe/ncnn_cv_postprocess_pipe.hpp"
#include "nanoai_ncnn/cv/pipe/ncnn_cv_preprocess_pipe.hpp"
#include "car_types.hpp"

namespace NanoAI_NCNN::Projects::CarDetectionExample
{
    inline auto make_car_pipeline(const std::string &param_path,
                                  const std::string &bin_path,
                                  const std::string &input_node = "images",
                                  const std::string &output_node = "output0",
                                  int num_threads = 2,
                                  bool use_vulkan = false)
    {
        const std::vector<std::string> class_names = {
            "Car",
            "Bus",
            "Truck",
            "Tram",
            "Two-wheeler",
            "Tricycle",
            "Pedestrian"};

        const NcnnModelSpec model_spec{
            param_path,
            bin_path,
            NcnnModelDomain::cv,
            num_threads,
            use_vulkan};

        return NanoAI_FLOW::make_pipeline_builder<NanoAI_FLOW::PipeForwardOrder::ordered>(0, 0)
            .add_pipe(CV::NcnnLoadPipe<>(model_spec))
            .add_pipe(CV::NcnnCvPreprocessPipe_LetterBox<>(960, 960))
            .add_pipe(CV::NcnnCvPreprocessPipe_RgbNormalize<>(true))
            .add_pipe(CV::NcnnCvInferPipe<>(input_node, output_node))
            .add_pipe(CV::NcnnCvPostprocessPipe_Yolo11<CarDetections>(
                class_names,
                0.25F,
                0.45F,
                true,
                false,
                true,
                [](const CV::NcnnCvYolo11Detections &detections)
                {
                    CarDetections out;
                    out.reserve(detections.size());
                    for (const auto &det : detections)
                    {
                        CarBBox box;
                        box.lx = det.x1;
                        box.ly = det.y1;
                        box.rx = det.x2;
                        box.ry = det.y2;
                        box.score = det.score;
                        box.class_id = det.class_id;
                        box.class_name = det.class_name;
                        out.push_back(std::move(box));
                    }
                    return out;
                }))
            .build();
    }

} // namespace NanoAI_NCNN::Projects::CarDetectionExample
