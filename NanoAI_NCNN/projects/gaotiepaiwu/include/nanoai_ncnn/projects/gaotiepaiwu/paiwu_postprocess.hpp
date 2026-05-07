#pragma once

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

#include "nanoai_ncnn/core/ncnn_types.hpp"
#include "nanoai_ncnn/projects/gaotiepaiwu/paiwu_types.hpp"

namespace NanoAI_NCNN::Projects::GaoTiePaiWu
{
    namespace detail
    {
        inline float sigmoid(float x)
        {
            return 1.0F / (1.0F + std::exp(-x));
        }

        inline float iou(const PaiwuBBox &a, const PaiwuBBox &b)
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

        inline void single_class_nms(const PaiwuDetections &input_boxes,
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

        inline void class_wise_nms(const PaiwuDetections &input_boxes,
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

        inline void keep_top_score_per_class(const PaiwuDetections &input_boxes,
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
    } // namespace detail

    inline PaiwuDetections decode_paiwu_detections(const NcnnInferResult &result)
    {
        constexpr float kConfThreshold = 0.25F;
        constexpr float kNmsThreshold = 0.45F;

        PaiwuDetections candidate_boxes;
        if (result.output.empty() || result.output.h <= 0 || result.output.w <= 0)
        {
            return candidate_boxes;
        }

        const int class_count = static_cast<int>(PaiwuType::Connect) + 1;
        const int feature_dim = 4 + class_count;

        bool parsed_yolo_head = false;
        int num_candidates = 0;
        std::vector<float> yolo_features;

        if (result.output.dims == 2)
        {
            if (result.output.h == feature_dim && result.output.w > 0)
            {
                num_candidates = result.output.w;
                yolo_features.assign(static_cast<NanoAI_FLOW::nanoai_usize>(feature_dim * num_candidates), 0.0F);
                for (int f = 0; f < feature_dim; ++f)
                {
                    const float *row = result.output.row(f);
                    std::copy(row, row + num_candidates, yolo_features.begin() + static_cast<NanoAI_FLOW::nanoai_usize>(f * num_candidates));
                }
                parsed_yolo_head = true;
            }
            else if (result.output.w == feature_dim && result.output.h > 0)
            {
                num_candidates = result.output.h;
                yolo_features.assign(static_cast<NanoAI_FLOW::nanoai_usize>(feature_dim * num_candidates), 0.0F);
                for (int i = 0; i < num_candidates; ++i)
                {
                    const float *det = result.output.row(i);
                    for (int f = 0; f < feature_dim; ++f)
                    {
                        yolo_features[static_cast<NanoAI_FLOW::nanoai_usize>(f * num_candidates + i)] = det[f];
                    }
                }
                parsed_yolo_head = true;
            }
        }
        else if (result.output.dims == 3)
        {
            if (result.output.c == feature_dim && result.output.h > 0 && result.output.w > 0)
            {
                num_candidates = result.output.h * result.output.w;
                yolo_features.assign(static_cast<NanoAI_FLOW::nanoai_usize>(feature_dim * num_candidates), 0.0F);
                for (int f = 0; f < feature_dim; ++f)
                {
                    const ncnn::Mat channel_mat = result.output.channel(f);
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
            else if (result.output.h == feature_dim && result.output.c > 0 && result.output.w > 0)
            {
                num_candidates = result.output.c * result.output.w;
                yolo_features.assign(static_cast<NanoAI_FLOW::nanoai_usize>(feature_dim * num_candidates), 0.0F);
                for (int c = 0; c < result.output.c; ++c)
                {
                    const ncnn::Mat channel_mat = result.output.channel(c);
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
            else if (result.output.w == feature_dim && result.output.c > 0 && result.output.h > 0)
            {
                num_candidates = result.output.c * result.output.h;
                yolo_features.assign(static_cast<NanoAI_FLOW::nanoai_usize>(feature_dim * num_candidates), 0.0F);
                for (int c = 0; c < result.output.c; ++c)
                {
                    const ncnn::Mat channel_mat = result.output.channel(c);
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
            const float inv_ratio = result.meta.ratio > 0.0F ? (1.0F / result.meta.ratio) : 1.0F;

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
                        score = detail::sigmoid(score);
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

                x1 = (x1 - static_cast<float>(result.meta.pad_x)) * inv_ratio;
                y1 = (y1 - static_cast<float>(result.meta.pad_y)) * inv_ratio;
                x2 = (x2 - static_cast<float>(result.meta.pad_x)) * inv_ratio;
                y2 = (y2 - static_cast<float>(result.meta.pad_y)) * inv_ratio;

                PaiwuBBox bbox;
                bbox.lx = std::clamp(x1, 0.0F, static_cast<float>(std::max(result.meta.src_width - 1, 0)));
                bbox.ly = std::clamp(y1, 0.0F, static_cast<float>(std::max(result.meta.src_height - 1, 0)));
                bbox.rx = std::clamp(x2, 0.0F, static_cast<float>(std::max(result.meta.src_width - 1, 0)));
                bbox.ry = std::clamp(y2, 0.0F, static_cast<float>(std::max(result.meta.src_height - 1, 0)));
                bbox.score = best_score;
                bbox.class_id = best_class_id;
                bbox.class_name = paiwu_class_name(best_class_id);

                if (bbox.rx <= bbox.lx || bbox.ry <= bbox.ly)
                {
                    continue;
                }

                candidate_boxes.push_back(std::move(bbox));
            }
        }
        else if (result.output.w >= 6)
        {
            const int num_detections = result.output.h;
            candidate_boxes.reserve(static_cast<NanoAI_FLOW::nanoai_usize>(num_detections));
            for (int i = 0; i < num_detections; ++i)
            {
                const float *det = result.output.row(i);

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
            return candidate_boxes;
        }

        PaiwuDetections nms_boxes;
        nms_boxes.reserve(candidate_boxes.size());
        detail::class_wise_nms(candidate_boxes, nms_boxes, kNmsThreshold);

        PaiwuDetections final_boxes;
        detail::keep_top_score_per_class(nms_boxes, final_boxes);
        return final_boxes;
    }

} // namespace NanoAI_NCNN::Projects::GaoTiePaiWu
