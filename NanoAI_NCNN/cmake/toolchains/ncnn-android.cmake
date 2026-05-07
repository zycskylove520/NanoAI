# Toolchain wrapper for NanoAI_NCNN Android cross compile.
set(ANDROID_ABI "arm64-v8a" CACHE STRING "Android ABI")
set(ANDROID_PLATFORM "android-26" CACHE STRING "Android API")
set(ANDROID_ARM_NEON ON CACHE BOOL "Android ARM NEON")
set(ANDROID_USE_LIBCXX ON CACHE BOOL "Android libc++")
set(ANDROID_STL "c++_static" CACHE STRING "Android STL")
