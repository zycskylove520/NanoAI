#pragma once

#include <string>

#include "nanoai_ncnn/core/ncnn_pipeline.hpp"
#include "nanoai_ncnn/cv/ncnn_cv_infer_pipe.hpp"
#include "nanoai_ncnn/cv/ncnn_cv_load_pipe.hpp"
#include "nanoai_ncnn/cv/ncnn_cv_postprocess_pipe.hpp"
#include "nanoai_ncnn/cv/ncnn_cv_preprocess_pipe.hpp"
#include "nanoai_ncnn/projects/car_project/car_postprocess.hpp"

namespace NanoAI_NCNN::Projects::CarProject
{
    using CarPipeline = NcnnPipeLine<
        CV::NcnnLoadPipe<>,
        CV::NcnnCvRgbPreprocessPipe<960, 960>,
        CV::NcnnCvInferPipe<>,
        CV::NcnnCvPostprocessPipe<CarDetections>>;

    inline CarPipeline make_car_pipeline(const std::string &name,
                                         const std::string &param_path,
                                         const std::string &bin_path,
                                         const std::string &input_node = "images",
                                         const std::string &output_node = "output0",
                                         int num_threads = 2,
                                         bool use_vulkan = false)
    {
        return CarPipeline(
            name,
            0,
            0,
            CV::NcnnLoadPipe<>(NcnnModelSpec{param_path, bin_path, NcnnModelDomain::cv, num_threads, use_vulkan}),
            CV::NcnnCvRgbPreprocessPipe<960, 960>(),
            CV::NcnnCvInferPipe<>(input_node, output_node),
            CV::NcnnCvPostprocessPipe<CarDetections>(decode_car_detections));
    }

} // namespace NanoAI_NCNN::Projects::CarProject
