// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: ncnn_cv_postprocess_pipe.hpp
// Brief: TODO - add file summary.
//

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "nanoai_flow/core/pipe.hpp"
#include "nanoai_ncnn/cv/ncnn_cv_types.hpp"

using NanoAI_FLOW::nanoai_u32;
using NanoAI_FLOW::NanoPipe;
using NanoAI_FLOW::PipeExecutionPolicy;

namespace NanoAI_NCNN::CV
{
    struct NcnnCvYolo11Detection
    {
        float x1{0.0F};
        float y1{0.0F};
        float x2{0.0F};
        float y2{0.0F};
        float score{0.0F};
        int class_id{-1};
        std::string class_name{};
    };

    using NcnnCvYolo11Detections = std::vector<NcnnCvYolo11Detection>;

    struct NcnnCvYolo11PostprocessOptions
    {
        float conf_threshold{0.25F};
        float nms_threshold{0.45F};
        bool classwise_nms{true};
        bool keep_top_score_per_class{false};
        bool apply_sigmoid_if_needed{true};
        std::vector<std::string> class_names{};
    };

    template <
        typename Out = NcnnCvInferResult,
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class NcnnCvPostprocessPipe : public NanoPipe<NcnnCvPostprocessPipe<Out, NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        using Callback = std::function<Out(const NcnnCvInferResult &)>;

        explicit NcnnCvPostprocessPipe(Callback callback = nullptr)
            : callback_(std::move(callback))
        {
        }

        Out on_run(const std::shared_ptr<NcnnRuntimeContext> &, const NcnnCvInferResult &result)
        {
            if (callback_)
            {
                return callback_(result);
            }

            if constexpr (std::is_same_v<Out, NcnnCvInferResult>)
            {
                return result;
            }

            throw std::invalid_argument("NcnnCvPostprocessPipe::on_run: callback is empty");
        }

    private:
        Callback callback_{};
    };

} // namespace NanoAI_NCNN::CV

namespace NanoAI_NCNN::CV
{
    template <
        typename Out = NcnnCvYolo11Detections,
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class NcnnCvPostprocessPipe_Yolo11 : public NanoPipe<NcnnCvPostprocessPipe_Yolo11<Out, NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        using Callback = std::function<Out(const NcnnCvYolo11Detections &)>;

        explicit NcnnCvPostprocessPipe_Yolo11(NcnnCvYolo11PostprocessOptions options = {}, Callback callback = nullptr)
            : options_(std::move(options)), callback_(std::move(callback))
        {
        }

        explicit NcnnCvPostprocessPipe_Yolo11(std::vector<std::string> class_names,
                                              float conf_threshold = 0.25F,
                                              float nms_threshold = 0.45F,
                                              bool classwise_nms = true,
                                              bool keep_top_score_per_class = false,
                                              bool apply_sigmoid_if_needed = true,
                                                                                            Callback callback = nullptr)
            : options_({conf_threshold, nms_threshold, classwise_nms, keep_top_score_per_class, true, std::move(class_names)}),
                            callback_(std::move(callback))
        {
            options_.apply_sigmoid_if_needed = apply_sigmoid_if_needed;
        }

        Out on_run(const std::shared_ptr<NcnnRuntimeContext> &, const NcnnCvInferResult &result)
        {
            NcnnCvYolo11Detections detections = decode_detections(result);
            if (callback_)
            {
                return callback_(detections);
            }

            if constexpr (std::is_same_v<Out, NcnnCvYolo11Detections>)
            {
                return detections;
            }

            throw std::invalid_argument("NcnnCvPostprocessPipe_Yolo11::on_run: converter is required for custom Out type");
        }

    private:
        static float sigmoid(float x)
        {
            return 1.0F / (1.0F + std::exp(-x));
        }

        static float iou(const NcnnCvYolo11Detection &lhs, const NcnnCvYolo11Detection &rhs)
        {
            const float inter_left = std::max(lhs.x1, rhs.x1);
            const float inter_top = std::max(lhs.y1, rhs.y1);
            const float inter_right = std::min(lhs.x2, rhs.x2);
            const float inter_bottom = std::min(lhs.y2, rhs.y2);

            const float inter_w = std::max(0.0F, inter_right - inter_left);
            const float inter_h = std::max(0.0F, inter_bottom - inter_top);
            const float inter_area = inter_w * inter_h;

            const float lhs_area = std::max(0.0F, lhs.x2 - lhs.x1) * std::max(0.0F, lhs.y2 - lhs.y1);
            const float rhs_area = std::max(0.0F, rhs.x2 - rhs.x1) * std::max(0.0F, rhs.y2 - rhs.y1);
            const float union_area = lhs_area + rhs_area - inter_area;

            if (union_area <= 0.0F)
            {
                return 0.0F;
            }

            return inter_area / union_area;
        }

        static std::string class_name_from_index(int class_id, const std::vector<std::string> &class_names)
        {
            if (class_id >= 0 && static_cast<std::size_t>(class_id) < class_names.size())
            {
                return class_names[static_cast<std::size_t>(class_id)];
            }

            return std::to_string(class_id);
        }

        static int infer_class_count(const ncnn::Mat &output, const std::vector<std::string> &class_names)
        {
            if (!class_names.empty())
            {
                return static_cast<int>(class_names.size());
            }

            if (output.dims == 2)
            {
                if (output.h > 4)
                {
                    return output.h - 4;
                }
                if (output.w > 4)
                {
                    return output.w - 4;
                }
            }
            else if (output.dims == 3)
            {
                if (output.c > 4)
                {
                    return output.c - 4;
                }
                if (output.h > 4)
                {
                    return output.h - 4;
                }
                if (output.w > 4)
                {
                    return output.w - 4;
                }
            }

            return 0;
        }

        static bool extract_yolo_features(const ncnn::Mat &output,
                                          int feature_dim,
                                          int &num_candidates,
                                          std::vector<float> &yolo_features)
        {
            num_candidates = 0;
            yolo_features.clear();

            if (output.dims != 2 || output.h != feature_dim || output.w <= 0)
            {
                return false;
            }

            num_candidates = output.w;
            yolo_features.assign(static_cast<std::size_t>(feature_dim * num_candidates), 0.0F);
            for (int f = 0; f < feature_dim; ++f)
            {
                const float *row = output.row(f);
                std::copy(row, row + num_candidates, yolo_features.begin() + static_cast<std::size_t>(f * num_candidates));
            }
            return true;
        }

        static NcnnCvYolo11Detection de_letterbox_detection(const NcnnCvYolo11Detection &in_box, const NcnnCvPadRatio &pr)
        {
            NcnnCvYolo11Detection out_box = in_box;
            if (pr.ratio <= 0.0F)
            {
                return out_box;
            }

            const float inv_ratio = 1.0F / pr.ratio;
            out_box.x1 = (in_box.x1 - static_cast<float>(pr.pad_x)) * inv_ratio;
            out_box.y1 = (in_box.y1 - static_cast<float>(pr.pad_y)) * inv_ratio;
            out_box.x2 = (in_box.x2 - static_cast<float>(pr.pad_x)) * inv_ratio;
            out_box.y2 = (in_box.y2 - static_cast<float>(pr.pad_y)) * inv_ratio;

            if (pr.src_width > 0 && pr.src_height > 0)
            {
                const float max_x = static_cast<float>(pr.src_width - 1);
                const float max_y = static_cast<float>(pr.src_height - 1);
                out_box.x1 = std::clamp(out_box.x1, 0.0F, max_x);
                out_box.y1 = std::clamp(out_box.y1, 0.0F, max_y);
                out_box.x2 = std::clamp(out_box.x2, 0.0F, max_x);
                out_box.y2 = std::clamp(out_box.y2, 0.0F, max_y);
            }

            return out_box;
        }

        static void apply_single_class_nms(const NcnnCvYolo11Detections &input_boxes,
                                           NcnnCvYolo11Detections &output_boxes,
                                           float nms_threshold)
        {
            NcnnCvYolo11Detections sorted_boxes = input_boxes;
            std::sort(sorted_boxes.begin(), sorted_boxes.end(), [](const NcnnCvYolo11Detection &lhs, const NcnnCvYolo11Detection &rhs)
                      { return lhs.score > rhs.score; });

            std::vector<bool> removed(sorted_boxes.size(), false);
            for (std::size_t i = 0; i < sorted_boxes.size(); ++i)
            {
                if (removed[i])
                {
                    continue;
                }

                output_boxes.push_back(sorted_boxes[i]);
                for (std::size_t j = i + 1; j < sorted_boxes.size(); ++j)
                {
                    if (!removed[j] && iou(sorted_boxes[i], sorted_boxes[j]) > nms_threshold)
                    {
                        removed[j] = true;
                    }
                }
            }
        }

        NcnnCvYolo11Detections decode_detections(const NcnnCvInferResult &result) const
        {
            NcnnCvYolo11Detections detections;
            if (result.output.empty() || result.output.w <= 0 || result.output.h <= 0)
            {
                return detections;
            }

            const int class_count = infer_class_count(result.output, options_.class_names);
            if (class_count <= 0)
            {
                return detections;
            }

            const int feature_dim = 4 + class_count;
            int num_candidates = 0;
            std::vector<float> yolo_features;

            // Merge decoding + confidence filtering in one pass to reduce temporary buffers.
            if (extract_yolo_features(result.output, feature_dim, num_candidates, yolo_features))
            {
                detections.reserve(static_cast<std::size_t>(num_candidates));

                bool class_need_sigmoid = false;
                if (options_.apply_sigmoid_if_needed && num_candidates > 0)
                {
                    for (int c = 0; c < class_count && !class_need_sigmoid; ++c)
                    {
                        const float value = yolo_features[static_cast<std::size_t>((4 + c) * num_candidates)];
                        class_need_sigmoid = (value < 0.0F || value > 1.0F);
                    }
                }

                for (int i = 0; i < num_candidates; ++i)
                {
                    const float cx = yolo_features[static_cast<std::size_t>(i)];
                    const float cy = yolo_features[static_cast<std::size_t>(num_candidates + i)];
                    const float w = yolo_features[static_cast<std::size_t>(2 * num_candidates + i)];
                    const float h = yolo_features[static_cast<std::size_t>(3 * num_candidates + i)];

                    int best_class_id = -1;
                    float best_score = 0.0F;
                    for (int c = 0; c < class_count; ++c)
                    {
                        float score = yolo_features[static_cast<std::size_t>((4 + c) * num_candidates + i)];
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

                    if (best_class_id < 0 || best_score < options_.conf_threshold)
                    {
                        continue;
                    }

                    NcnnCvYolo11Detection det;
                    det.x1 = cx - 0.5F * w;
                    det.y1 = cy - 0.5F * h;
                    det.x2 = cx + 0.5F * w;
                    det.y2 = cy + 0.5F * h;
                    det.score = best_score;
                    det.class_id = best_class_id;
                    det.class_name = class_name_from_index(best_class_id, options_.class_names);
                    det = de_letterbox_detection(det, result.pr);

                    if (det.x2 <= det.x1 || det.y2 <= det.y1)
                    {
                        continue;
                    }

                    detections.push_back(std::move(det));
                }
            }

            if (detections.empty())
            {
                return detections;
            }

            NcnnCvYolo11Detections nms_output;
            nms_output.reserve(detections.size());
            if (options_.classwise_nms)
            {
                std::unordered_map<int, NcnnCvYolo11Detections> boxes_by_class;
                boxes_by_class.reserve(16);
                for (const auto &box : detections)
                {
                    boxes_by_class[box.class_id].push_back(box);
                }

                for (const auto &entry : boxes_by_class)
                {
                    apply_single_class_nms(entry.second, nms_output, options_.nms_threshold);
                }
            }
            else
            {
                apply_single_class_nms(detections, nms_output, options_.nms_threshold);
            }

            if (!options_.keep_top_score_per_class)
            {
                std::sort(nms_output.begin(), nms_output.end(), [](const NcnnCvYolo11Detection &lhs, const NcnnCvYolo11Detection &rhs)
                          { return lhs.score > rhs.score; });
                return nms_output;
            }

            std::unordered_map<int, NcnnCvYolo11Detection> best_box_by_class;
            best_box_by_class.reserve(16);
            for (const auto &box : nms_output)
            {
                auto it = best_box_by_class.find(box.class_id);
                if (it == best_box_by_class.end() || box.score > it->second.score)
                {
                    best_box_by_class[box.class_id] = box;
                }
            }

            NcnnCvYolo11Detections reduced;
            reduced.reserve(best_box_by_class.size());
            for (const auto &entry : best_box_by_class)
            {
                reduced.push_back(entry.second);
            }
            std::sort(reduced.begin(), reduced.end(), [](const NcnnCvYolo11Detection &lhs, const NcnnCvYolo11Detection &rhs)
                      { return lhs.score > rhs.score; });
            return reduced;
        }

    private:
        NcnnCvYolo11PostprocessOptions options_{};
        Callback callback_{};
    };
} // namespace NanoAI_NCNN::CV
