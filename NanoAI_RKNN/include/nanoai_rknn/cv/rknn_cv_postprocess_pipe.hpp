// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: rknn_cv_postprocess_pipe.hpp
// Brief: 提供 RKNN 视觉推理结果的通用后处理 pipe 与 YOLO11 检测解码实现。
//
// Design notes:
// - 通用后处理 pipe 允许用户注入任意转换逻辑，默认直接透传推理结果。
// - YOLO11 后处理假设首个输出张量已经按 [4 + num_classes, num_boxes] 排列并可转成 float。
//

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <string>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "nanoai_flow/core/pipe.hpp"
#include "nanoai_rknn/cv/rknn_cv_types.hpp"

namespace NanoAI_RKNN::CV
{
    using NanoAI_FLOW::NanoPipe;
    using NanoAI_FLOW::nanoai_u32;
    using NanoAI_FLOW::PipeExecutionPolicy;

    // 单个检测框使用左上/右下角坐标表示，便于直接做 NMS 与可视化绘制。
    struct RknnCvYolo11Detection
    {
        float x1{0.0F};
        float y1{0.0F};
        float x2{0.0F};
        float y2{0.0F};
        float score{0.0F};
        int class_id{-1};
        std::string class_name{};
    };

    using RknnCvYolo11Detections = std::vector<RknnCvYolo11Detection>;

    // YOLO11 后处理参数集中管理阈值、类别名与 NMS 行为，便于示例和业务代码复用。
    struct RknnCvYolo11PostprocessOptions
    {
        float conf_threshold{0.25F};
        float nms_threshold{0.45F};
        bool classwise_nms{true};
        bool keep_top_score_per_class{false};
        bool apply_sigmoid_if_needed{true};
        std::vector<std::string> class_names{};
    };

    template <
        typename Out = RknnCvInferResult,
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class RknnPostprocessPipe : public NanoPipe<RknnPostprocessPipe<Out, NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        using Callback = std::function<Out(const RknnCvInferResult &)>;

        explicit RknnPostprocessPipe(Callback callback = nullptr)
            : postprocess_cb_(std::move(callback))
        {
        }

        Out on_run(const std::shared_ptr<Helper::RKNNAIContext> &, const RknnCvInferResult &result)
        {
            return postprocess_impl(result);
        }

        Out on_run(const std::shared_ptr<Helper::RKNNAIContext> &, RknnCvInferResult &&result)
        {
            return postprocess_impl(result);
        }

    private:
        Out postprocess_impl(const RknnCvInferResult &result)
        {
            if (postprocess_cb_)
            {
                return postprocess_cb_(result);
            }

            // 未提供回调时仅允许透传原始推理结果，避免对自定义输出类型产生隐式且不可控的转换。
            if constexpr (std::is_same_v<Out, RknnCvInferResult>)
            {
                return result;
            }

            throw std::invalid_argument("RknnPostprocessPipe::on_run: postprocess callback is empty");
        }

        Callback postprocess_cb_{};
    };

    template <
        typename Out = RknnCvInferResult,
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    using RknnHostPostprocessPipe = RknnPostprocessPipe<Out, NumThreads, Policy, DedicatedPoolSize>;

    template <
        typename Out = RknnCvInferResult,
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    using RknnZeroCopyPostprocessPipe = RknnPostprocessPipe<Out, NumThreads, Policy, DedicatedPoolSize>;

    template <
        typename Out = RknnCvYolo11Detections,
        nanoai_u32 NumThreads = 0,
        PipeExecutionPolicy Policy = PipeExecutionPolicy::shared_pool,
        nanoai_u32 DedicatedPoolSize = 0>
    class RknnCvPostprocessPipe_Yolo11 : public NanoPipe<RknnCvPostprocessPipe_Yolo11<Out, NumThreads, Policy, DedicatedPoolSize>, NumThreads, Policy, DedicatedPoolSize>
    {
    public:
        using Callback = std::function<Out(const RknnCvYolo11Detections &)>;

        explicit RknnCvPostprocessPipe_Yolo11(RknnCvYolo11PostprocessOptions options = {}, Callback callback = nullptr)
            : options_(std::move(options)), callback_(std::move(callback))
        {
        }

        explicit RknnCvPostprocessPipe_Yolo11(std::vector<std::string> class_names,
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

        Out on_run(const std::shared_ptr<Helper::RKNNAIContext> &, const RknnCvInferResult &result)
        {
            RknnCvYolo11Detections detections = decode_detections(result);
            if (callback_)
            {
                return callback_(detections);
            }

            if constexpr (std::is_same_v<Out, RknnCvYolo11Detections>)
            {
                return detections;
            }

            throw std::invalid_argument("RknnCvPostprocessPipe_Yolo11::on_run: converter is required for custom Out type");
        }

        Out on_run(const std::shared_ptr<Helper::RKNNAIContext> &, RknnCvInferResult &&result)
        {
            return on_run(nullptr, result);
        }

    private:
        static float sigmoid(float x)
        {
            return 1.0F / (1.0F + std::exp(-x));
        }

        // IoU 仅用于 NMS 抑制阶段，默认按轴对齐矩形计算，适配常见 YOLO 检测框输出。
        static float iou(const RknnCvYolo11Detection &lhs, const RknnCvYolo11Detection &rhs)
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

        static int infer_class_count(const RknnOutputTensor &output, const std::vector<std::string> &class_names)
        {
            // 优先使用显式 class_names，避免对不同导出布局的模型做脆弱猜测。
            if (!class_names.empty())
            {
                return static_cast<int>(class_names.size());
            }

            if (output.shape.size() == 2)
            {
                if (output.shape[0] > 4)
                {
                    return output.shape[0] - 4;
                }
                if (output.shape[1] > 4)
                {
                    return output.shape[1] - 4;
                }
            }

            return 0;
        }

        static bool extract_yolo_features(const RknnOutputTensor &output,
                                          int feature_dim,
                                          int &num_candidates,
                                          std::vector<float> &yolo_features)
        {
            num_candidates = 0;
            yolo_features.clear();

            if (output.shape.size() != 2 || output.shape[0] != feature_dim || output.shape[1] <= 0)
            {
                return false;
            }

            const std::size_t expected_size = static_cast<std::size_t>(output.shape[0] * output.shape[1]);
            if (output.data.size() < expected_size)
            {
                return false;
            }

            num_candidates = output.shape[1];
            yolo_features.assign(output.data.begin(), output.data.begin() + static_cast<std::ptrdiff_t>(expected_size));
            return true;
        }

        // NMS 先按分数降序保留最优框，再抑制与其 IoU 超阈值的候选框。
        static void apply_single_class_nms(const RknnCvYolo11Detections &input_boxes,
                                           RknnCvYolo11Detections &output_boxes,
                                           float nms_threshold)
        {
            RknnCvYolo11Detections sorted_boxes = input_boxes;
            std::sort(sorted_boxes.begin(), sorted_boxes.end(), [](const RknnCvYolo11Detection &lhs, const RknnCvYolo11Detection &rhs)
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

        RknnCvYolo11Detections decode_detections(const RknnCvInferResult &result) const
        {
            RknnCvYolo11Detections detections;
            if (result.outputs.empty())
            {
                return detections;
            }

            // 当前实现默认解析首个输出张量；若未来模型导出为多头输出，可在此处扩展聚合逻辑。
            const RknnOutputTensor &output = result.outputs.front();
            const int class_count = infer_class_count(output, options_.class_names);
            if (class_count <= 0)
            {
                return detections;
            }

            const int feature_dim = 4 + class_count;
            int num_candidates = 0;
            std::vector<float> yolo_features;

            if (extract_yolo_features(output, feature_dim, num_candidates, yolo_features))
            {
                detections.reserve(static_cast<std::size_t>(num_candidates));

                // 某些导出结果的类别分数仍是 logits，这里按需一次性检测后统一套 sigmoid。
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

                    RknnCvYolo11Detection det;
                    det.x1 = cx - 0.5F * w;
                    det.y1 = cy - 0.5F * h;
                    det.x2 = cx + 0.5F * w;
                    det.y2 = cy + 0.5F * h;
                    det.score = best_score;
                    det.class_id = best_class_id;
                    det.class_name = class_name_from_index(best_class_id, options_.class_names);

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

            RknnCvYolo11Detections nms_output;
            nms_output.reserve(detections.size());
            if (options_.classwise_nms)
            {
                std::unordered_map<int, RknnCvYolo11Detections> boxes_by_class;
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
                std::sort(nms_output.begin(), nms_output.end(), [](const RknnCvYolo11Detection &lhs, const RknnCvYolo11Detection &rhs)
                          { return lhs.score > rhs.score; });
                return nms_output;
            }

            // 某些分类展示场景只需要每类一个最佳框，这里在 NMS 之后再做一次按类别收敛。
            std::unordered_map<int, RknnCvYolo11Detection> best_box_by_class;
            best_box_by_class.reserve(16);
            for (const auto &box : nms_output)
            {
                auto it = best_box_by_class.find(box.class_id);
                if (it == best_box_by_class.end() || box.score > it->second.score)
                {
                    best_box_by_class[box.class_id] = box;
                }
            }

            RknnCvYolo11Detections reduced;
            reduced.reserve(best_box_by_class.size());
            for (const auto &entry : best_box_by_class)
            {
                reduced.push_back(entry.second);
            }
            std::sort(reduced.begin(), reduced.end(), [](const RknnCvYolo11Detection &lhs, const RknnCvYolo11Detection &rhs)
                      { return lhs.score > rhs.score; });
            return reduced;
        }

    private:
        RknnCvYolo11PostprocessOptions options_{};
        Callback callback_{};
    };

} // namespace NanoAI_RKNN
