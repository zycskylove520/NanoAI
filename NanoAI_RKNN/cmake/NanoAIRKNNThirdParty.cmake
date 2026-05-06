include_guard(GLOBAL)

# ======================================================
# 3rdparty 依赖库配置
# ======================================================
set(NANOAI_THIRD_PARTY_DIR ${NANOAI_PROJECT_ROOT}/3rdparty)
set(NANOAI_RKNN_THIRD_PARTY_DIR ${NANOAI_THIRD_PARTY_DIR}/rknn)

set(NANOAI_RKNN_WITH_RKNN_RUNTIME ON CACHE BOOL "Enable RKNN runtime dependency" FORCE)
set(NANOAI_RKNN_WITH_RGA ON CACHE BOOL "Enable RGA dependency" FORCE)
set(NANOAI_RKNN_WITH_OPENCV ON CACHE BOOL "Enable OpenCV dependency" FORCE)
set(NANOAI_RKNN_WITH_STB_IMAGE ON CACHE BOOL "Enable stb_image dependency" FORCE)
set(NANOAI_RKNN_WITH_JPEG_TURBO ON CACHE BOOL "Enable jpeg_turbo dependency" FORCE)
set(NANOAI_RKNN_WITH_UTILS ON CACHE BOOL "Enable third-party utils sources" FORCE)

if(NANOAI_RKNN_WITH_UTILS AND NOT NANOAI_RKNN_WITH_RGA)
	message(WARNING "NANOAI_RKNN_WITH_UTILS requires NANOAI_RKNN_WITH_RGA=ON; disabling utils sources.")
	set(NANOAI_RKNN_WITH_UTILS OFF)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/thirdparty/RKNNRuntime.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/thirdparty/RGA.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/thirdparty/OpenCV.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/thirdparty/STBImage.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/thirdparty/JPEGTurbo.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/thirdparty/Utils.cmake)