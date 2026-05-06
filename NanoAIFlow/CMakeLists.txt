cmake_minimum_required(VERSION 3.16)

project(NanoAIFlow VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(NANOAIFLOW_BUILD_TESTS "Build NanoAIFlow tests" OFF)
option(NANOAIFLOW_BUILD_EXAMPLES "Build NanoAIFlow examples" OFF)
option(NANOAIFLOW_ENABLE_CPACK "Enable CPack package generation" OFF)

find_package(Threads REQUIRED)

add_library(NanoAIFlow INTERFACE)
add_library(NanoAI::Flow ALIAS NanoAIFlow)

target_compile_features(NanoAIFlow INTERFACE cxx_std_20)
target_include_directories(NanoAIFlow
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/thread-pool/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(NanoAIFlow INTERFACE Threads::Threads)

set(NANOAIFLOW_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/NanoAIFlow" CACHE STRING "Install path for NanoAIFlow CMake package files")

install(TARGETS NanoAIFlow
    EXPORT NanoAIFlowTargets
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES 3rdparty/thread-pool/include/BS_thread_pool.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/NanoAIFlowConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NanoAIFlowConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/NanoAIFlowConfig.cmake"
    INSTALL_DESTINATION ${NANOAIFLOW_INSTALL_CMAKEDIR}
)

install(EXPORT NanoAIFlowTargets
    FILE NanoAIFlowTargets.cmake
    NAMESPACE NanoAI::
    DESTINATION ${NANOAIFLOW_INSTALL_CMAKEDIR}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/NanoAIFlowConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/NanoAIFlowConfigVersion.cmake"
    DESTINATION ${NANOAIFLOW_INSTALL_CMAKEDIR}
)

if(NANOAIFLOW_ENABLE_CPACK)
    set(CPACK_PACKAGE_NAME "NanoAIFlow")
    set(CPACK_PACKAGE_VENDOR "NanoAI")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_GENERATOR "TGZ;ZIP")
    include(CPack)
endif()

if(NANOAIFLOW_BUILD_EXAMPLES)
    add_executable(nanoaiflow_benchmark examples/perf_benchmark.cpp)
    target_link_libraries(nanoaiflow_benchmark PRIVATE NanoAIFlow)
endif()

if(NANOAIFLOW_BUILD_TESTS)
    enable_testing()
    add_executable(nanoaiflow_test_basic_chain test/test_basic_chain.cpp)
    target_link_libraries(nanoaiflow_test_basic_chain PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.BasicChain COMMAND nanoaiflow_test_basic_chain)

    add_executable(nanoaiflow_test_tuple_forwarding test/test_tuple_forwarding.cpp)
    target_link_libraries(nanoaiflow_test_tuple_forwarding PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.TupleForwarding COMMAND nanoaiflow_test_tuple_forwarding)

    add_executable(nanoaiflow_test_load_once_concurrency test/test_load_once_concurrency.cpp)
    target_link_libraries(nanoaiflow_test_load_once_concurrency PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.LoadOnceConcurrency COMMAND nanoaiflow_test_load_once_concurrency)

    add_executable(nanoaiflow_test_pointer test/test_pointer.cpp)
    target_link_libraries(nanoaiflow_test_pointer PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.Pointer COMMAND nanoaiflow_test_pointer)

    add_executable(nanoaiflow_test_reference test/test_reference.cpp)
    target_link_libraries(nanoaiflow_test_reference PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.Reference COMMAND nanoaiflow_test_reference)

    add_executable(nanoaiflow_test_const_reference test/test_const_reference.cpp)
    target_link_libraries(nanoaiflow_test_const_reference PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.ConstReference COMMAND nanoaiflow_test_const_reference)

    add_executable(nanoaiflow_test_pool_performance test/test_pool_performance.cpp)
    target_link_libraries(nanoaiflow_test_pool_performance PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.PoolPerformance COMMAND nanoaiflow_test_pool_performance)

    add_executable(nanoaiflow_test_order_consistency_high_concurrency test/test_order_consistency_high_concurrency.cpp)
    target_link_libraries(nanoaiflow_test_order_consistency_high_concurrency PRIVATE NanoAIFlow)
    add_test(NAME NanoAIFlow.OrderConsistencyHighConcurrency COMMAND nanoaiflow_test_order_consistency_high_concurrency)
endif()
