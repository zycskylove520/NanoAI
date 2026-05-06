#include "nanoai_ncnn/projects/gaotiepaiwu/paiwu_detector.hpp"

#ifndef __ANDROID__

namespace NanoAI_NCNN::Projects::GaoTiePaiWu
{
    PaiwuDetector::PaiwuDetector(const std::string &ncnn_param_path,
                                 const std::string &ncnn_bin_path,
                                 int num_threads)
        : pipeline_(make_paiwu_pipeline("gaotiepaiwu_pipeline",
                                        ncnn_param_path,
                                        ncnn_bin_path,
                                        "in0",
                                        "out0",
                                        num_threads,
                                        false))
    {
    }

    PaiwuDetector::~PaiwuDetector() = default;

    void PaiwuDetector::detect(const cv::Mat &input_image, PaiwuDetections &paiwu_results)
    {
        paiwu_results = pipeline_.run(CV::NcnnPipelineInput{input_image});
    }

} // namespace NanoAI_NCNN::Projects::GaoTiePaiWu

#endif
