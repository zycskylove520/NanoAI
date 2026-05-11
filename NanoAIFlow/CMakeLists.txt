# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: CMakeLists.txt
# Brief: NanoAIFlow 框架始终 install，examples 可选构建，所有依赖 INTERFACE 链接。
#
# Build/Package policy:
# - NanoAIFlow 是 header-only INTERFACE 目标，默认只安装框架本体。
# - tests/examples 始终为可选项，不影响 install 包内容。
# - 安装后通过 find_package(NanoAIFlow CONFIG REQUIRED) 引入。

cmake_minimum_required(VERSION 3.16)

project(NanoAIFlow VERSION 1.0.0 LANGUAGES CXX)

# 提供标准安装目录变量（CMAKE_INSTALL_INCLUDEDIR/CMAKE_INSTALL_LIBDIR 等）。
include(GNUInstallDirs)
# 提供 CMake 包配置文件生成功能。
include(CMakePackageConfigHelpers)


# 可选开关：测试、示例（主库始终 install，examples 仅在 ON 时构建）。
option(NANOAIFLOW_BUILD_TESTS "Build NanoAIFlow tests (optional)" OFF)
option(NANOAIFLOW_BUILD_EXAMPLES "Build NanoAIFlow examples (optional)" OFF)
option(NANOAIFLOW_ENABLE_CPACK "Enable CPack package generation (optional)" OFF)

# 测试超时（秒）：用于避免线程池相关测试在异常场景下无限等待。
set(NANOAIFLOW_TEST_TIMEOUT_SECONDS "60" CACHE STRING "CTest timeout in seconds for NanoAIFlow tests")

## 头文件库目标与命名空间别名目标（始终 install，INTERFACE 链接依赖）
add_library(NanoAIFlow INTERFACE)
add_library(NanoAI::Flow ALIAS NanoAIFlow)

# 对使用方强制要求 C++20（thread_pool/pipeline 使用 C++20 并发原语）。
target_compile_features(NanoAIFlow INTERFACE cxx_std_20)

# BUILD_INTERFACE: 源码树内开发时包含 include/
# INSTALL_INTERFACE: 安装后使用 <prefix>/include
target_include_directories(NanoAIFlow
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

## 安装 CMake 包配置文件目录
set(NANOAIFLOW_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/NanoAIFlow" CACHE STRING "Install path for NanoAIFlow CMake package files")

# 导出 INTERFACE 目标到安装树，供下游 find_package 导入。
install(TARGETS NanoAIFlow
    EXPORT NanoAIFlowTargets
)

# 安装全部头文件。
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 生成版本文件，约束为同主版本兼容。
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/NanoAIFlowConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# 根据模板生成 NanoAIFlowConfig.cmake。
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NanoAIFlowConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/NanoAIFlowConfig.cmake"
    INSTALL_DESTINATION ${NANOAIFLOW_INSTALL_CMAKEDIR}
)

# 安装导出目标文件。
install(EXPORT NanoAIFlowTargets
    FILE NanoAIFlowTargets.cmake
    NAMESPACE NanoAI::
    DESTINATION ${NANOAIFLOW_INSTALL_CMAKEDIR}
)

# 安装 config/version 文件，完成 find_package 所需最小集合。
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/NanoAIFlowConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/NanoAIFlowConfigVersion.cmake"
    DESTINATION ${NANOAIFLOW_INSTALL_CMAKEDIR}
)

if(NANOAIFLOW_ENABLE_CPACK)
    # 可选打包配置（不影响普通构建/安装流程）。
    set(CPACK_PACKAGE_NAME "NanoAIFlow")
    set(CPACK_PACKAGE_VENDOR "NanoAI")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_GENERATOR "TGZ;ZIP")
    include(CPack)
endif()

if(NANOAIFLOW_BUILD_EXAMPLES)
    # 入门示例程序：覆盖管线创建/执行与 copy/move 语义示例。
    add_executable(nanoaiflow_quickstart examples/pipeline_quickstart.cpp)
    target_link_libraries(nanoaiflow_quickstart PRIVATE NanoAIFlow)

    # 单文件性能示例程序：用于粗粒度吞吐观察。
    add_executable(nanoaiflow_benchmark examples/perf_benchmark.cpp)
    target_link_libraries(nanoaiflow_benchmark PRIVATE NanoAIFlow)
endif()

if(NANOAIFLOW_BUILD_TESTS)
    # CTest 注册区：所有测试均链接同一 INTERFACE 框架目标。
    enable_testing()

    # 功能正确性测试：链式执行和结果校验。
    add_executable(nanoaiflow_test_basic_chain test/test_basic_chain.cpp)
    target_link_libraries(nanoaiflow_test_basic_chain PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.BasicChain COMMAND nanoaiflow_test_basic_chain)

    # Tuple 展开与参数转发语义测试。
    add_executable(nanoaiflow_test_tuple_forwarding test/test_tuple_forwarding.cpp)
    target_link_libraries(nanoaiflow_test_tuple_forwarding PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.TupleForwarding COMMAND nanoaiflow_test_tuple_forwarding)

    # load-once 并发场景行为测试。
    add_executable(nanoaiflow_test_load_once_concurrency test/test_load_once_concurrency.cpp)
    target_link_libraries(nanoaiflow_test_load_once_concurrency PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.LoadOnceConcurrency COMMAND nanoaiflow_test_load_once_concurrency)

    # 指针传递语义测试。
    add_executable(nanoaiflow_test_pointer test/test_pointer.cpp)
    target_link_libraries(nanoaiflow_test_pointer PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.Pointer COMMAND nanoaiflow_test_pointer)

    # 引用传递语义测试。
    add_executable(nanoaiflow_test_reference test/test_reference.cpp)
    target_link_libraries(nanoaiflow_test_reference PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.Reference COMMAND nanoaiflow_test_reference)

    # const 引用传递语义测试。
    add_executable(nanoaiflow_test_const_reference test/test_const_reference.cpp)
    target_link_libraries(nanoaiflow_test_const_reference PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.ConstReference COMMAND nanoaiflow_test_const_reference)

    # 线程池/运行时性能回归测试（用于发现明显退化与卡顿风险）。
    add_executable(nanoaiflow_test_pool_performance test/test_pool_performance.cpp)
    target_link_libraries(nanoaiflow_test_pool_performance PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.PoolPerformance COMMAND nanoaiflow_test_pool_performance)

    # 高并发有序一致性测试。
    add_executable(nanoaiflow_test_order_consistency_high_concurrency test/test_order_consistency_high_concurrency.cpp)
    target_link_libraries(nanoaiflow_test_order_consistency_high_concurrency PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.OrderConsistencyHighConcurrency COMMAND nanoaiflow_test_order_consistency_high_concurrency)

    # 线程池异常兜底与异常计数行为测试。
    add_executable(nanoaiflow_test_thread_pool_exception test/test_thread_pool_exception.cpp)
    target_link_libraries(nanoaiflow_test_thread_pool_exception PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.ThreadPoolException COMMAND nanoaiflow_test_thread_pool_exception)

    # 背压策略与可观测性能力测试。
    add_executable(nanoaiflow_test_backpressure_and_observability test/test_backpressure_and_observability.cpp)
    target_link_libraries(nanoaiflow_test_backpressure_and_observability PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.BackpressureAndObservability COMMAND nanoaiflow_test_backpressure_and_observability)

    # 统一设置测试超时，尤其保护线程池相关用例，避免卡死时无限等待。
    set_tests_properties(
        NanoAIFlow.BasicChain
        NanoAIFlow.TupleForwarding
        NanoAIFlow.LoadOnceConcurrency
        NanoAIFlow.Pointer
        NanoAIFlow.Reference
        NanoAIFlow.ConstReference
        NanoAIFlow.PoolPerformance
        NanoAIFlow.OrderConsistencyHighConcurrency
        NanoAIFlow.ThreadPoolException
        NanoAIFlow.BackpressureAndObservability
        PROPERTIES TIMEOUT ${NANOAIFLOW_TEST_TIMEOUT_SECONDS}
    )

    # 注册 test_cancel_and_timeout 测试，纳入 CTest 并设置超时。
    add_executable(nanoaiflow_test_cancel_and_timeout test/test_cancel_and_timeout.cpp)
    target_link_libraries(nanoaiflow_test_cancel_and_timeout PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.CancelAndTimeout COMMAND nanoaiflow_test_cancel_and_timeout)
    set_tests_properties(NanoAIFlow.CancelAndTimeout PROPERTIES TIMEOUT ${NANOAIFLOW_TEST_TIMEOUT_SECONDS})
endif()
