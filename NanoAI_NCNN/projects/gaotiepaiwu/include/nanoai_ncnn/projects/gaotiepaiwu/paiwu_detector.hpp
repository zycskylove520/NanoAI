#pragma once

#include <string>

#include <opencv2/opencv.hpp>

#ifdef __ANDROID__
#include "ncnn/mat.h"
#include "ncnn/net.h"
#else
#include "nanoai_ncnn/cv/ncnn_cv_types.hpp"
#include "nanoai_ncnn/projects/gaotiepaiwu/paiwu_pipeline.hpp"
#endif

#include "nanoai_ncnn/projects/gaotiepaiwu/paiwu_types.hpp"

namespace NanoAI_NCNN::Projects::GaoTiePaiWu
{
    class PaiwuDetector
    {
    public:
        explicit PaiwuDetector(const std::string &ncnn_param_path,
                               const std::string &ncnn_bin_path,
                               int num_threads = 2);

        ~PaiwuDetector();

        void detect(const cv::Mat &input_image, PaiwuDetections &paiwu_results);

    private:
    #ifdef __ANDROID__
        void preprocess(const cv::Mat &input_image, ncnn::Mat &output_blob, float &ratio, int &pad_x, int &pad_y);

        static void letterbox(const cv::Mat &src,
                      cv::Mat &dst,
                      int target_width,
                      int target_height,
                      float &ratio,
                      int &pad_x,
                      int &pad_y);

        private:
        int input_height_ = 640;
        int input_width_ = 640;

        ncnn::Net net_;
    #else
        PaiwuPipeline pipeline_;
    #endif
    };

} // namespace NanoAI_NCNN::Projects::GaoTiePaiWu
