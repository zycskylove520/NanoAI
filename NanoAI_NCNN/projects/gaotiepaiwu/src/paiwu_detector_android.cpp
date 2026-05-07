#include "nanoai_ncnn/projects/gaotiepaiwu/paiwu_detector.hpp"
#include "nanoai_flow/core/types.h"

#ifdef __ANDROID__

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_map>

namespace NanoAI_NCNN::Projects::GaoTiePaiWu
{
    namespace
    {
        constexpr float kConfThreshold = 0.25F;
        constexpr float kNmsThreshold = 0.45F;

        float sigmoid(float x)
        {
            return 1.0F / (1.0F + std::exp(-x));
        }

        float iou(const PaiwuBBox &a, const PaiwuBBox &b)
        {
            const float inter_left = std::max(a.lx, b.lx);
            const float inter_top = std::max(a.ly, b.ly);
            const float inter_right = std::min(a.rx, b.rx);
            const float inter_bottom = std::min(a.ry, b.ry);

            const float inter_w = std::max(0.0F, inter_right - inter_left);
            const float inter_h = std::max(0.0F, inter_bottom - inter_top);
            const float inter_area = inter_w * inter_h;

            const float area_a = std::max(0.0F, a.rx - a.lx) * std::max(0.0F, a.ry - a.ly);
            const float area_b = std::max(0.0F, b.rx - b.lx) * std::max(0.0F, b.ry - b.ly);
            const float union_area = area_a + area_b - inter_area;

            if (union_area <= 0.0F)
            {
                return 0.0F;
            }

            return inter_area / union_area;
        }

        void single_class_nms(const PaiwuDetections &input_boxes,
                              PaiwuDetections &output_boxes,
                              float nms_threshold)
        {
            PaiwuDetections sorted_boxes = input_boxes;
            std::sort(sorted_boxes.begin(), sorted_boxes.end(), [](const PaiwuBBox &lhs, const PaiwuBBox &rhs) {
                return lhs.score > rhs.score;
            });

            std::vector<bool> removed(sorted_boxes.size(), false);
            for (NanoAI_FLOW::nanoai_usize i = 0; i < sorted_boxes.size(); ++i)
            {
                if (removed[i])
                {
                    continue;
                }

                output_boxes.push_back(sorted_boxes[i]);
                for (NanoAI_FLOW::nanoai_usize j = i + 1; j < sorted_boxes.size(); ++j)
                {
                    if (removed[j])
                    {
                        continue;
                    }

                    if (iou(sorted_boxes[i], sorted_boxes[j]) > nms_threshold)
                    {
                        removed[j] = true;
                    }
                }
            }
        }

        void class_wise_nms(const PaiwuDetections &input_boxes,
                            PaiwuDetections &output_boxes,
                            float nms_threshold)
        {
            std::unordered_map<int, PaiwuDetections> boxes_by_class;
            boxes_by_class.reserve(8);
            for (const auto &box : input_boxes)
            {
                boxes_by_class[box.class_id].push_back(box);
            }

            output_boxes.clear();
            for (const auto &kv : boxes_by_class)
            {
                single_class_nms(kv.second, output_boxes, nms_threshold);
            }
        }

        void keep_top_score_per_class(const PaiwuDetections &input_boxes,
                                      PaiwuDetections &output_boxes)
        {
            std::unordered_map<int, PaiwuBBox> best_box_by_class;
            best_box_by_class.reserve(8);
            for (const auto &box : input_boxes)
            {
                auto it = best_box_by_class.find(box.class_id);
                if (it == best_box_by_class.end() || box.score > it->second.score)
                {
                    best_box_by_class[box.class_id] = box;
                }
            }

            output_boxes.clear();
            output_boxes.reserve(best_box_by_class.size());
            for (const auto &kv : best_box_by_class)
            {
                output_boxes.push_back(kv.second);
            }

            std::sort(output_boxes.begin(), output_boxes.end(), [](const PaiwuBBox &lhs, const PaiwuBBox &rhs) {
                return lhs.score > rhs.score;
            });
        }
    } // namespace

    PaiwuDetector::PaiwuDetector(const std::string &ncnn_param_path,
                                 const std::string &ncnn_bin_path,
                                 int num_threads)
    {
        net_.opt.num_threads = num_threads;

        const int load_param_result = net_.load_param(ncnn_param_path.c_str());
        if (load_param_result != 0)
        {
            throw std::runtime_error("PaiwuDetector: failed to load param file");
        }

        const int load_bin_result = net_.load_model(ncnn_bin_path.c_str());
        if (load_bin_result != 0)
        {
            throw std::runtime_error("PaiwuDetector: failed to load bin file");
        }
    }

    PaiwuDetector::~PaiwuDetector()
    {
        net_.clear();
    }

    void PaiwuDetector::preprocess(const cv::Mat &input_image,
                                   ncnn::Mat &output_blob,
                                   float &ratio,
                                   int &pad_x,
                                   int &pad_y)
    {
        cv::Mat letterboxed_image;
        letterbox(input_image, letterboxed_image, input_width_, input_height_, ratio, pad_x, pad_y);

        cv::cvtColor(letterboxed_image, letterboxed_image, cv::COLOR_BGR2RGB);

        output_blob = ncnn::Mat::from_pixels(letterboxed_image.data,
                                             ncnn::Mat::PIXEL_RGB,
                                             input_width_,
                                             input_height_);
        const float mean_vals[3] = {0.0F, 0.0F, 0.0F};
        const float norm_vals[3] = {0.007843F, 0.007843F, 0.007843F};
        output_blob.substract_mean_normalize(mean_vals, norm_vals);
    }

    void PaiwuDetector::detect(const cv::Mat &input_image, PaiwuDetections &paiwu_results)
    {
        paiwu_results.clear();

        ncnn::Mat input_blob;
        float ratio = 1.0F;
        int pad_x = 0;
        int pad_y = 0;
        preprocess(input_image, input_blob, ratio, pad_x, pad_y);

        ncnn::Extractor extractor = net_.create_extractor();
        extractor.input("in0", input_blob);

        ncnn::Mat output_blob;
        extractor.extract("out0", output_blob);

        if (output_blob.h <= 0 || output_blob.w <= 0)
        {
            return;
        }

        PaiwuDetections candidate_boxes;
        const int class_count = static_cast<int>(PaiwuType::Connect) + 1;
        const int feature_dim = 4 + class_count;

        bool parsed_yolo_head = false;
        int num_candidates = 0;
        std::vector<float> yolo_features;

        if (output_blob.dims == 2)
        {
            if (output_blob.h == feature_dim && output_blob.w > 0)
            {
                num_candidates = output_blob.w;
                yolo_features.assign(static_cast<NanoAI_FLOW::nanoai_usize>(feature_dim * num_candidates), 0.0F);
                for (int f = 0; f < feature_dim; ++f)
                {
                    const float *row = output_blob.row(f);
                    std::copy(row, row + num_candidates, yolo_features.begin() + static_cast<NanoAI_FLOW::nanoai_usize>(f * num_candidates));
                }
                parsed_yolo_head = true;
            }
            else if (output_blob.w == feature_dim && output_blob.h > 0)
            {
                num_candidates = output_blob.h;
                yolo_features.assign(static_cast<NanoAI_FLOW::nanoai_usize>(feature_dim * num_candidates), 0.0F);
                for (int i = 0; i < num_candidates; ++i)
                {
                    const float *det = output_blob.row(i);
                    for (int f = 0; f < feature_dim; ++f)
                    {
                        yolo_features[static_cast<NanoAI_FLOW::nanoai_usize>(f * num_candidates + i)] = det[f];
                    }
                }
                parsed_yolo_head = true;
            }
        }
        else if (output_blob.dims == 3)
        {
            if (output_blob.c == feature_dim && output_blob.h > 0 && output_blob.w > 0)
            {
                num_candidates = output_blob.h * output_blob.w;
                yolo_features.assign(static_cast<NanoAI_FLOW::nanoai_usize>(feature_dim * num_candidates), 0.0F);
                for (int f = 0; f < feature_dim; ++f)
                {
                    const ncnn::Mat channel_mat = output_blob.channel(f);
                    for (int y = 0; y < channel_mat.h; ++y)
                    {
                        const float *row = channel_mat.row(y);
                        for (int x = 0; x < channel_mat.w; ++x)
                        {
                            const int i = y * channel_mat.w + x;
                            yolo_features[static_cast<NanoAI_FLOW::nanoai_usize>(f * num_candidates + i)] = row[x];
                        }
                    }
                }
                parsed_yolo_head = true;
            }
            else if (output_blob.h == feature_dim && output_blob.c > 0 && output_blob.w > 0)
            {
                num_candidates = output_blob.c * output_blob.w;
                yolo_features.assign(static_cast<NanoAI_FLOW::nanoai_usize>(feature_dim * num_candidates), 0.0F);
                for (int c = 0; c < output_blob.c; ++c)
                {
                    const ncnn::Mat channel_mat = output_blob.channel(c);
                    for (int f = 0; f < feature_dim; ++f)
                    {
                        const float *row = channel_mat.row(f);
                        for (int x = 0; x < channel_mat.w; ++x)
                        {
                            const int i = c * channel_mat.w + x;
                            yolo_features[static_cast<NanoAI_FLOW::nanoai_usize>(f * num_candidates + i)] = row[x];
                        }
                    }
                }
                parsed_yolo_head = true;
            }
            else if (output_blob.w == feature_dim && output_blob.c > 0 && output_blob.h > 0)
            {
                num_candidates = output_blob.c * output_blob.h;
                yolo_features.assign(static_cast<NanoAI_FLOW::nanoai_usize>(feature_dim * num_candidates), 0.0F);
                for (int c = 0; c < output_blob.c; ++c)
                {
                    const ncnn::Mat channel_mat = output_blob.channel(c);
                    for (int y = 0; y < channel_mat.h; ++y)
                    {
                        const float *row = channel_mat.row(y);
                        const int i = c * channel_mat.h + y;
                        for (int f = 0; f < feature_dim; ++f)
                        {
                            yolo_features[static_cast<NanoAI_FLOW::nanoai_usize>(f * num_candidates + i)] = row[f];
                        }
                    }
                }
                parsed_yolo_head = true;
            }
        }

        if (parsed_yolo_head)
        {
            candidate_boxes.reserve(static_cast<NanoAI_FLOW::nanoai_usize>(num_candidates));
            const float inv_ratio = ratio > 0.0F ? (1.0F / ratio) : 1.0F;

            bool class_need_sigmoid = false;
            for (int c = 0; c < class_count && !class_need_sigmoid; ++c)
            {
                const float v = yolo_features[static_cast<NanoAI_FLOW::nanoai_usize>((4 + c) * num_candidates)];
                class_need_sigmoid = (v < 0.0F || v > 1.0F);
            }

            for (int i = 0; i < num_candidates; ++i)
            {
                const float cx = yolo_features[static_cast<NanoAI_FLOW::nanoai_usize>(i)];
                const float cy = yolo_features[static_cast<NanoAI_FLOW::nanoai_usize>(num_candidates + i)];
                const float w = yolo_features[static_cast<NanoAI_FLOW::nanoai_usize>(2 * num_candidates + i)];
                const float h = yolo_features[static_cast<NanoAI_FLOW::nanoai_usize>(3 * num_candidates + i)];

                int best_class_id = -1;
                float best_score = 0.0F;
                for (int c = 0; c < class_count; ++c)
                {
                    float score = yolo_features[static_cast<NanoAI_FLOW::nanoai_usize>((4 + c) * num_candidates + i)];
                    if (class_need_sigmoid)
                    {
                        score = sigmoid(score);
                    }

                    if (score > best_score)
                    {
                        best_score = score;
                        best_class_id = c;
                    }
                }

                if (best_class_id < 0 || best_score < kConfThreshold)
                {
                    continue;
                }

                float x1 = cx - 0.5F * w;
                float y1 = cy - 0.5F * h;
                float x2 = cx + 0.5F * w;
                float y2 = cy + 0.5F * h;

                x1 = (x1 - static_cast<float>(pad_x)) * inv_ratio;
                y1 = (y1 - static_cast<float>(pad_y)) * inv_ratio;
                x2 = (x2 - static_cast<float>(pad_x)) * inv_ratio;
                y2 = (y2 - static_cast<float>(pad_y)) * inv_ratio;

                PaiwuBBox bbox;
                bbox.lx = std::clamp(x1, 0.0F, static_cast<float>(std::max(input_image.cols - 1, 0)));
                bbox.ly = std::clamp(y1, 0.0F, static_cast<float>(std::max(input_image.rows - 1, 0)));
                bbox.rx = std::clamp(x2, 0.0F, static_cast<float>(std::max(input_image.cols - 1, 0)));
                bbox.ry = std::clamp(y2, 0.0F, static_cast<float>(std::max(input_image.rows - 1, 0)));
                bbox.score = best_score;
                bbox.class_id = best_class_id;
                bbox.class_name = paiwu_class_name(bbox.class_id);

                if (bbox.rx <= bbox.lx || bbox.ry <= bbox.ly)
                {
                    continue;
                }

                candidate_boxes.push_back(std::move(bbox));
            }
        }
        else if (output_blob.w >= 6)
        {
            const int num_detections = output_blob.h;
            candidate_boxes.reserve(static_cast<NanoAI_FLOW::nanoai_usize>(num_detections));
            for (int i = 0; i < num_detections; ++i)
            {
                const float *det = output_blob.row(i);

                PaiwuBBox bbox;
                bbox.lx = det[0];
                bbox.ly = det[1];
                bbox.rx = det[2];
                bbox.ry = det[3];
                bbox.score = det[4];
                bbox.class_id = static_cast<int>(det[5]);
                bbox.class_name = paiwu_class_name(bbox.class_id);

                if (bbox.score < kConfThreshold)
                {
                    continue;
                }

                candidate_boxes.push_back(std::move(bbox));
            }
        }

        if (candidate_boxes.empty())
        {
            return;
        }

        PaiwuDetections nms_boxes;
        nms_boxes.reserve(candidate_boxes.size());
        class_wise_nms(candidate_boxes, nms_boxes, kNmsThreshold);
        keep_top_score_per_class(nms_boxes, paiwu_results);
    }

    void PaiwuDetector::letterbox(const cv::Mat &src,
                                  cv::Mat &dst,
                                  int target_width,
                                  int target_height,
                                  float &ratio,
                                  int &pad_x,
                                  int &pad_y)
    {
        const int original_width = src.cols;
        const int original_height = src.rows;

        ratio = std::min(static_cast<float>(target_width) / static_cast<float>(original_width),
                         static_cast<float>(target_height) / static_cast<float>(original_height));

        const int new_unpad_width = static_cast<int>(std::round(static_cast<float>(original_width) * ratio));
        const int new_unpad_height = static_cast<int>(std::round(static_cast<float>(original_height) * ratio));

        pad_x = (target_width - new_unpad_width) / 2;
        pad_y = (target_height - new_unpad_height) / 2;

        if (original_width != new_unpad_width || original_height != new_unpad_height)
        {
            cv::resize(src, dst, cv::Size(new_unpad_width, new_unpad_height), 0.0, 0.0, cv::INTER_LINEAR);
        }
        else
        {
            dst = src.clone();
        }

        const int top = static_cast<int>(std::round(static_cast<float>(pad_y) - 0.1F));
        const int bottom = static_cast<int>(std::round(static_cast<float>(pad_y) + 0.1F));
        const int left = static_cast<int>(std::round(static_cast<float>(pad_x) - 0.1F));
        const int right = static_cast<int>(std::round(static_cast<float>(pad_x) + 0.1F));

        cv::copyMakeBorder(dst, dst, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(114, 114, 114));
    }

} // namespace NanoAI_NCNN::Projects::GaoTiePaiWu

#endif
