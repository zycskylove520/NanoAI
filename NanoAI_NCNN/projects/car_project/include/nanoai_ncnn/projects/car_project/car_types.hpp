#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace NanoAI_NCNN::Projects::CarProject
{
    enum class CarType : std::uint8_t
    {
        car = 0,
        bus = 1,
        truck = 2,
        tram = 3,
        two_wheeler = 4,
        tricycle = 5,
        pedestrian = 6,
    };

    struct CarBBox
    {
        float lx{0.0F};
        float ly{0.0F};
        float rx{0.0F};
        float ry{0.0F};
        float score{0.0F};
        int class_id{-1};
        std::string class_name{"Unknown"};
    };

    inline const char *car_class_name(int class_id)
    {
        switch (class_id)
        {
        case 0:
            return "Car";
        case 1:
            return "Bus";
        case 2:
            return "Truck";
        case 3:
            return "Tram";
        case 4:
            return "Two-wheeler";
        case 5:
            return "Tricycle";
        case 6:
            return "Pedestrian";
        default:
            return "Unknown";
        }
    }

    using CarDetections = std::vector<CarBBox>;

} // namespace NanoAI_NCNN::Projects::CarProject
