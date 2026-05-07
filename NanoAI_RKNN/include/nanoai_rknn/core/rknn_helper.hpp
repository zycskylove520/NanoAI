// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: rknn_helper.hpp
// Brief: TODO - add file summary.
//

#pragma once

#include <stdio.h>
#include <string>
#include <cstring>

#include <rknn_api.h>
#include <dma_alloc.h>
#include "file_utils.h"
#include "nanoai_flow/core/types.h"

namespace NanoAI_RKNN::Helper
{
    /**
     * @brief RKNN 模型推理上下文结构体
     *        管理模型句柄、输入输出属性、张量内存等所有推理相关资源
     */
    struct RKNNAIContext
    {
        rknn_context ctx_ = 0;                     // RKNN 模型上下文句柄
        rknn_input_output_num io_num_ = {};        // 模型输入输出张量总数
        rknn_tensor_attr *input_attrs_ = nullptr;  // 模型输入张量属性数组
        rknn_tensor_attr *output_attrs_ = nullptr; // 模型输出张量属性数组

        int model_channel_ = 0; // 模型输入通道数
        int model_width_ = 0;   // 模型输入宽度
        int model_height_ = 0;  // 模型输入高度

        rknn_tensor_mem **input_mems_ = nullptr;  // 零拷贝输入张量内存数组
        rknn_tensor_mem **output_mems_ = nullptr; // 零拷贝输出张量内存数组
    };

    /**
     * @brief DMA 连续内存上下文结构体
     *        用于管理物理连续的非缓存 DMA 缓冲区
     */
    struct RKNNDMAContext
    {
        int dma_fd_ = 0;             // DMA 缓冲区文件描述符
        NanoAI_FLOW::nanoai_u8 *dma_buf_ = nullptr; // DMA 缓冲区虚拟地址指针
        int dma_buf_size_ = 0;       // DMA 缓冲区总大小
    };

    /**
     * @brief 打印 RKNN 张量属性信息
     * @param attr 待打印的张量属性结构体
     */
    inline void DumpTensorAttr(rknn_tensor_attr attr)
    {
        printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
               "zp=%d, scale=%f\n",
               attr.index, attr.name, attr.n_dims, attr.dims[0], attr.dims[1], attr.dims[2], attr.dims[3],
               attr.n_elems, attr.size, get_format_string(attr.fmt), get_type_string(attr.type),
               get_qnt_type_string(attr.qnt_type), attr.zp, attr.scale);
    }

    /**
     * @brief 从文件加载 RKNN 模型并初始化模型上下文
     * @param ai_ctx 输出参数，RKNN 推理上下文
     * @param model_path 模型文件路径（.rknn 格式）
     * @return 成功返回 0，失败返回负数
     */
    inline int LoadModel(RKNNAIContext &ai_ctx, const std::string &model_path)
    {
        // 加载模型
        char *model = nullptr;
        int model_len = read_data_from_file(model_path.c_str(), &model);
        if (model == nullptr || model_len <= 0)
        {
            printf("load_model fail! model is nullptr or len=%d\n", model_len);
            return -1;
        }

        // 初始化rknn引擎
        int ret = rknn_init(&ai_ctx.ctx_, model, model_len, 0, nullptr);
        free(model);
        if (ret < 0)
        {
            printf("rknn_init fail! ret=%d\n", ret);
            return -1;
        }

        return 0;
    }

    /**
     * @brief 初始化模型信息：查询输入输出数量与张量属性，解析模型输入尺寸
     * @param ai_ctx RKNN 推理上下文
     * @return 成功返回 0，失败返回负数
     */
    inline int InitModel(RKNNAIContext &ai_ctx)
    {
        // 获取模型输入输出数量
        int ret = rknn_query(ai_ctx.ctx_, RKNN_QUERY_IN_OUT_NUM, &ai_ctx.io_num_, sizeof(ai_ctx.io_num_));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        printf("model input num: %d, output num: %d\n", ai_ctx.io_num_.n_input, ai_ctx.io_num_.n_output);

        // 模型输入
        printf("input tensors:\n");
        rknn_tensor_attr input_attrs[ai_ctx.io_num_.n_input];
        for (int i = 0; i < ai_ctx.io_num_.n_input; i++)
        {
            input_attrs[i].index = i;
            ret = rknn_query(ai_ctx.ctx_, RKNN_QUERY_INPUT_ATTR, &input_attrs[i], sizeof(rknn_tensor_attr));
            if (ret != RKNN_SUCC)
            {
                printf("rknn_query fail! ret=%d\n", ret);
                return -1;
            }

            DumpTensorAttr(input_attrs[i]);

            // 解析输入类型
            auto input_attr = input_attrs[i];
            if (input_attr.fmt == RKNN_TENSOR_NCHW)
            {
                ai_ctx.model_channel_ = input_attr.dims[1];
                ai_ctx.model_height_ = input_attr.dims[2];
                ai_ctx.model_width_ = input_attr.dims[3];
            }
            else if (input_attr.fmt == RKNN_TENSOR_NHWC)
            {
                ai_ctx.model_channel_ = input_attr.dims[3];
                ai_ctx.model_height_ = input_attr.dims[1];
                ai_ctx.model_width_ = input_attr.dims[2];
            }
            else
            {
                printf("unsupported input format!\n");
                return -1;
            }
        }

        ai_ctx.input_attrs_ = (rknn_tensor_attr *)malloc(ai_ctx.io_num_.n_input * sizeof(rknn_tensor_attr));
        if (ai_ctx.input_attrs_ == nullptr)
        {
            printf("tensor attr malloc fail!\n");
            return -1;
        }
        memcpy(ai_ctx.input_attrs_, input_attrs, ai_ctx.io_num_.n_input * sizeof(rknn_tensor_attr));

        // 模型输出
        printf("output tensors:\n");
        rknn_tensor_attr output_attrs[ai_ctx.io_num_.n_output];
        for (int i = 0; i < ai_ctx.io_num_.n_output; i++)
        {
            output_attrs[i].index = i;
            ret = rknn_query(ai_ctx.ctx_, RKNN_QUERY_OUTPUT_ATTR, &output_attrs[i], sizeof(rknn_tensor_attr));
            if (ret != RKNN_SUCC)
            {
                printf("rknn_query fail! ret=%d\n", ret);
                return -1;
            }

            DumpTensorAttr(output_attrs[i]);
        }

        ai_ctx.output_attrs_ = (rknn_tensor_attr *)malloc(ai_ctx.io_num_.n_output * sizeof(rknn_tensor_attr));
        if (ai_ctx.output_attrs_ == nullptr)
        {
            printf("tensor attr malloc fail!\n");
            return -1;
        }
        memcpy(ai_ctx.output_attrs_, output_attrs, ai_ctx.io_num_.n_output * sizeof(rknn_tensor_attr));

        return 0;
    }

    /**
     * @brief 创建零拷贝输入张量，绑定 RKNN 输入内存以提升数据传输效率
     * @param ai_ctx RKNN 推理上下文
     * @return 成功返回 0，失败返回负数
     */
    inline int CreateZeroCopyInputTensor(RKNNAIContext &ai_ctx)
    {
        int ret;

        if (ai_ctx.io_num_.n_input <= 0)
        {
            printf("create zero copy input tensor fail!\n");
            return -1;
        }

        ai_ctx.input_mems_ = (rknn_tensor_mem **)malloc(ai_ctx.io_num_.n_input * sizeof(rknn_tensor_mem *));
        if (ai_ctx.input_mems_ == nullptr)
        {
            printf("tensor mem malloc fail!\n");
            return -1;
        }

        for (int i = 0; i < ai_ctx.io_num_.n_input; ++i)
        {
            ai_ctx.input_mems_[i] = rknn_create_mem(ai_ctx.ctx_, ai_ctx.input_attrs_[i].size_with_stride);
            ret = rknn_set_io_mem(ai_ctx.ctx_, ai_ctx.input_mems_[i], &ai_ctx.input_attrs_[i]);
            if (ret < 0)
            {
                printf("input_mems rknn_set_io_mem fail! ret=%d\n", ret);
                return -1;
            }
        }

        return 0;
    }

    /**
     * @brief 创建零拷贝输出张量，绑定 RKNN 输出内存
     * @param ai_ctx RKNN 推理上下文
     * @return 成功返回 0，失败返回负数
     */
    inline int CreateZeroCopyOutputTensor(RKNNAIContext &ai_ctx)
    {
        int ret;

        if (ai_ctx.io_num_.n_output <= 0)
        {
            printf("create zero copy output tensor fail!\n");
            return -1;
        }

        ai_ctx.output_mems_ = (rknn_tensor_mem **)malloc(ai_ctx.io_num_.n_output * sizeof(rknn_tensor_mem *));
        if (ai_ctx.output_mems_ == nullptr)
        {
            printf("tensor mem malloc fail!\n");
            return -1;
        }

        for (int i = 0; i < ai_ctx.io_num_.n_output; ++i)
        {
            ai_ctx.output_mems_[i] = rknn_create_mem(ai_ctx.ctx_, ai_ctx.output_attrs_[i].size_with_stride);
            ret = rknn_set_io_mem(ai_ctx.ctx_, ai_ctx.output_mems_[i], &ai_ctx.output_attrs_[i]);
            if (ret < 0)
            {
                printf("output_mems rknn_set_io_mem fail! ret=%d\n", ret);
                return -1;
            }
        }

        return 0;
    }

    /**
     * @brief 释放 RKNNAIContext 所有资源，包括张量内存、模型句柄、属性内存
     * @param ai_ctx 待释放的 RKNN 推理上下文
     * @return 成功返回 0
     */
    inline int ReleaseAIContext(RKNNAIContext &ai_ctx)
    {
        int ret;

        // 销毁输入内存
        if (ai_ctx.input_mems_ != nullptr)
        {
            for (int i = 0; i < ai_ctx.io_num_.n_input; i++)
            {
                if (ai_ctx.input_mems_[i] != nullptr)
                {
                    ret = rknn_destroy_mem(ai_ctx.ctx_, ai_ctx.input_mems_[i]);
                    if (ret != RKNN_SUCC)
                    {
                        printf("rknn_destroy_mem input fail! ret=%d\n", ret);
                    }
                    ai_ctx.input_mems_[i] = nullptr;
                }
            }
            free(ai_ctx.input_mems_);
            ai_ctx.input_mems_ = nullptr;
        }

        // 销毁输出内存
        if (ai_ctx.output_mems_ != nullptr)
        {
            for (int i = 0; i < ai_ctx.io_num_.n_output; i++)
            {
                if (ai_ctx.output_mems_[i] != nullptr)
                {
                    ret = rknn_destroy_mem(ai_ctx.ctx_, ai_ctx.output_mems_[i]);
                    if (ret != RKNN_SUCC)
                    {
                        printf("rknn_destroy_mem output fail! ret=%d\n", ret);
                    }
                    ai_ctx.output_mems_[i] = nullptr;
                }
            }
            free(ai_ctx.output_mems_);
            ai_ctx.output_mems_ = nullptr;
        }

        // 销毁 ctx
        if (ai_ctx.ctx_ != 0)
        {
            ret = rknn_destroy(ai_ctx.ctx_);
            if (ret != RKNN_SUCC)
            {
                printf("rknn_destroy fail! ret=%d\n", ret);
            }
            ai_ctx.ctx_ = 0; // 必须清零
        }

        // 释放属性内存
        if (ai_ctx.input_attrs_ != nullptr)
        {
            free(ai_ctx.input_attrs_);
            ai_ctx.input_attrs_ = nullptr;
        }

        if (ai_ctx.output_attrs_ != nullptr)
        {
            free(ai_ctx.output_attrs_);
            ai_ctx.output_attrs_ = nullptr;
        }

        // 重置所有结构体，防止二次释放崩溃
        ai_ctx.io_num_ = {};
        ai_ctx.model_channel_ = 0;
        ai_ctx.model_width_ = 0;
        ai_ctx.model_height_ = 0;

        return 0;
    }

    /**
     * @brief 创建非缓存类型 DMA 连续内存缓冲区
     * @param dma_ctx 输出参数，DMA 内存上下文
     * @param buf_size 申请的内存大小
     * @return 成功返回 0，失败返回负数
     */
    inline int CreateDMABuf(RKNNDMAContext &dma_ctx, int buf_size)
    {
        int ret = dma_buf_alloc(DMA_HEAP_UNCACHE_PATH, buf_size, &dma_ctx.dma_fd_, (void **)&dma_ctx.dma_buf_);
        if (ret < 0 || !dma_ctx.dma_buf_)
        {
            printf("alloc src dma_heap buffer failed!\n");
            return -1;
        }
        dma_ctx.dma_buf_size_ = buf_size;
        memset(dma_ctx.dma_buf_, 0x00, buf_size);

        return 0;
    }

    /**
     * @brief 释放 DMA 内存上下文，关闭文件描述符并释放虚拟地址
     * @param dma_ctx 待释放的 DMA 内存上下文
     */
    inline void ReleaseDMAContext(RKNNDMAContext &dma_ctx)
    {
        if (dma_ctx.dma_buf_ && dma_ctx.dma_fd_ > 0)
        {
            dma_buf_free(dma_ctx.dma_buf_size_, &dma_ctx.dma_fd_, dma_ctx.dma_buf_);
            dma_ctx.dma_buf_ = nullptr;
            dma_ctx.dma_fd_ = 0;
            dma_ctx.dma_buf_size_ = 0;
        }
    }

} // namespace rknn_helper