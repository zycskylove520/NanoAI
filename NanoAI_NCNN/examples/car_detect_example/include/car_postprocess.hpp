#pragma once

#include <algorithm>
#include <cmath>

#include "nanoai_ncnn/core/ncnn_types.hpp"
#include "car_types.hpp"

namespace NanoAI_NCNN::Projects::CarDetectionExample
{
    inline CarDetections decode_car_detections(const NcnnInferResult &result)
    {
        CarDetections detections;

        if (result.output.empty())
        {
            return detections;
        }

        if (result.output.w < 6)
        {
            return detections;
        }

        const int num_detections = result.output.h;
        detections.reserve(static_cast<NanoAI_FLOW::nanoai_usize>(num_detections));

        for (int i = 0; i < num_detections; ++i)
        {
            const float *row = result.output.row(i);
            if (!row)
            {
                continue;
            }

            CarBBox box;
            box.lx = row[0];
            box.ly = row[1];
            box.rx = row[2];
            box.ry = row[3];
            box.score = row[4];
            box.class_id = static_cast<int>(row[5]);
            box.class_name = car_class_name(box.class_id);

            const float ratio = std::max(result.meta.ratio, 1e-6F);
            box.lx = (box.lx - static_cast<float>(result.meta.pad_x)) / ratio;
            box.ly = (box.ly - static_cast<float>(result.meta.pad_y)) / ratio;
            box.rx = (box.rx - static_cast<float>(result.meta.pad_x)) / ratio;
            box.ry = (box.ry - static_cast<float>(result.meta.pad_y)) / ratio;

            box.lx = std::clamp(box.lx, 0.0F, static_cast<float>(std::max(result.meta.src_width - 1, 0)));
            box.ly = std::clamp(box.ly, 0.0F, static_cast<float>(std::max(result.meta.src_height - 1, 0)));
            box.rx = std::clamp(box.rx, 0.0F, static_cast<float>(std::max(result.meta.src_width - 1, 0)));
            box.ry = std::clamp(box.ry, 0.0F, static_cast<float>(std::max(result.meta.src_height - 1, 0)));

            detections.emplace_back(std::move(box));
        }

        return detections;
    }

} // namespace NanoAI_NCNN::Projects::CarProject
