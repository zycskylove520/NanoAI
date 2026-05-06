#pragma once

/**
 * @struct Point
 * @brief 二维坐标点结构体（用于关键点）
 */
struct Point
{
    float x, y; // x/y 坐标
};

/**
 * @struct bbox
 * @brief 人脸检测输出结果（框+置信度+5个关键点）
 */
struct bbox
{
    float lx, ly, rx, ry; // 人脸框：左上角、右下角坐标（原图）
    float score;          // 人脸置信度
    Point landmark[5];    // 5个人脸关键点：左眼、右眼、鼻尖、左嘴角、右嘴角
};

/**
 * @struct box
 * @brief 模型Anchor先验框（内部使用）
 */
struct box
{
    float cx, cy, w, h; // 中心坐标 + 宽高（归一化）
};