#pragma once

#include <string>

#include "nanoai_ncnn/core/ncnn_pipeline.hpp"
#include "nanoai_ncnn/cv/ncnn_cv_infer_pipe.hpp"
#include "nanoai_ncnn/cv/ncnn_cv_load_pipe.hpp"
#include "nanoai_ncnn/cv/ncnn_cv_postprocess_pipe.hpp"
#include "nanoai_ncnn/cv/ncnn_cv_preprocess_pipe.hpp"
#include "nanoai_ncnn/projects/gaotiepaiwu/paiwu_postprocess.hpp"

namespace NanoAI_NCNN::Projects::GaoTiePaiWu
{
    using PaiwuPipeline = NcnnPipeLine<
        CV::NcnnLoadPipe<>,
        CV::NcnnCvRgbPreprocessPipe<640, 640>,
        CV::NcnnCvInferPipe<>,
        CV::NcnnCvPostprocessPipe<PaiwuDetections>>;

    inline PaiwuPipeline make_paiwu_pipeline(const std::string &name,
                                             const std::string &param_path,
                                             const std::string &bin_path,
                                             const std::string &input_node = "in0",
                                             const std::string &output_node = "out0",
                                             int num_threads = 2,
                                             bool use_vulkan = false)
    {
        return PaiwuPipeline(
            name,
            0,
            0,
            CV::NcnnLoadPipe<>(NcnnModelSpec{param_path, bin_path, NcnnModelDomain::cv, num_threads, use_vulkan}),
            CV::NcnnCvRgbPreprocessPipe<640, 640>(),
            CV::NcnnCvInferPipe<>(input_node, output_node),
            CV::NcnnCvPostprocessPipe<PaiwuDetections>(decode_paiwu_detections));
    }

} // namespace NanoAI_NCNN::Projects::GaoTiePaiWu
