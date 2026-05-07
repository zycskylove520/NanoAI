cmake_minimum_required(VERSION 3.16)

project(NanoAI VERSION 1.0.0 LANGUAGES NONE)

# Optional monorepo entry. Keep module builds independent by default.
option(NANOAI_BUILD_FLOW "Build NanoAIFlow subproject" ON)
option(NANOAI_BUILD_RKNN "Build NanoAI_RKNN subproject" OFF)
option(NANOAI_BUILD_NCNN "Build NanoAI_NCNN subproject" OFF)

set(NANOAI_NANOAIFLOW_PREFIX "" CACHE PATH "Optional NanoAIFlow install prefix for RKNN/NCNN")

message(STATUS "NanoAI monorepo options:")
message(STATUS "  NANOAI_BUILD_FLOW=${NANOAI_BUILD_FLOW}")
message(STATUS "  NANOAI_BUILD_RKNN=${NANOAI_BUILD_RKNN}")
message(STATUS "  NANOAI_BUILD_NCNN=${NANOAI_BUILD_NCNN}")

if(NANOAI_BUILD_FLOW)
    add_subdirectory(NanoAIFlow)
endif()

if(NANOAI_BUILD_RKNN)
    if(NANOAI_NANOAIFLOW_PREFIX AND NOT NANOAIFLOW_ROOT)
        set(NANOAIFLOW_ROOT "${NANOAI_NANOAIFLOW_PREFIX}" CACHE PATH "NanoAIFlow install prefix" FORCE)
    endif()
    add_subdirectory(NanoAI_RKNN)
endif()

if(NANOAI_BUILD_NCNN)
    if(NANOAI_NANOAIFLOW_PREFIX AND NOT NANOAIFLOW_ROOT)
        set(NANOAIFLOW_ROOT "${NANOAI_NANOAIFLOW_PREFIX}" CACHE PATH "NanoAIFlow install prefix" FORCE)
    endif()
    add_subdirectory(NanoAI_NCNN)
endif()
