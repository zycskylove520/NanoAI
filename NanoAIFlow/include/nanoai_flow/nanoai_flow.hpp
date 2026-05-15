// SPDX-License-Identifier: Apache-2.0
//
// Copyright (c) NanoAI
//
// File: nanoai_flow.hpp
// Brief: NanoAIFlow 对外统一入口头文件。
//
// Design notes:
// - 通过单一 include 暴露核心类型与常用 pipe 适配器，降低接入门槛。
// - 该头仅做聚合导出，不引入运行时状态。
// - 下游若追求最小编译依赖，可按需直接 include 子头。

#pragma once

// Core runtime utilities.
// 统一入口适合示例和小型项目；大型工程若关心编译时间，可改为按需包含子头。
#include "core/copy.hpp"
#include "core/defines.hpp"
#include "core/pipe.hpp"
#include "core/pipeline.hpp"
#include "core/timer.hpp"

// Common pipe adapters.
#include "pipes/model_pipe.hpp"
#include "pipes/process_pipe.hpp"
