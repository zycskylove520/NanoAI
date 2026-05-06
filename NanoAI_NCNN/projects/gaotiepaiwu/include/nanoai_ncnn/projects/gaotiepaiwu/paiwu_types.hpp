#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace NanoAI_NCNN::Projects::GaoTiePaiWu
{
    enum class PaiwuType : int
    {
        OpenAction = 0,
        CloseAction = 1,
        PaiwuAction = 2,
        Pipe = 3,
        ValvePort = 4,
        Connect = 5
    };

    struct PaiwuBBox
    {
        float lx = 0.0F;
        float ly = 0.0F;
        float rx = 0.0F;
        float ry = 0.0F;
        float score = 0.0F;
        int class_id = 0;
        std::string class_name;
    };

    inline const char *paiwu_class_name(int class_id)
    {
        switch (class_id)
        {
        case static_cast<int>(PaiwuType::OpenAction):
            return "Open Action";
        case static_cast<int>(PaiwuType::CloseAction):
            return "Close Action";
        case static_cast<int>(PaiwuType::PaiwuAction):
            return "Paiwu Action";
        case static_cast<int>(PaiwuType::Pipe):
            return "Pipe";
        case static_cast<int>(PaiwuType::ValvePort):
            return "Valve Port";
        case static_cast<int>(PaiwuType::Connect):
            return "Connect";
        default:
            return "Unknown";
        }
    }

    using PaiwuDetections = std::vector<PaiwuBBox>;

} // namespace NanoAI_NCNN::Projects::GaoTiePaiWu
