include_guard(GLOBAL)

# ======================================================
# Third-party dependency config
# ======================================================
set(NANOAI_THIRD_PARTY_DIR ${NANOAI_PROJECT_ROOT}/3rdparty)
set(NANOAI_NCNN_THIRD_PARTY_DIR ${NANOAI_THIRD_PARTY_DIR})

set(NANOAI_NCNN_WITH_NCNN ON CACHE BOOL "Enable NCNN dependency" FORCE)
set(NANOAI_NCNN_WITH_OPENCV ON CACHE BOOL "Enable OpenCV dependency" FORCE)

include(${CMAKE_CURRENT_LIST_DIR}/thirdparty/NCNNRuntime.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/thirdparty/OpenCV.cmake)
