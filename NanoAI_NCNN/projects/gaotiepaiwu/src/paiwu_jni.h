#ifndef PAIWU_JNI_H
#define PAIWU_JNI_H

#ifdef __ANDROID__

#include <jni.h>

#ifdef __cplusplus
extern "C"
{
#endif

JNIEXPORT void JNICALL Java_com_yitu_paiwu_PaiwuJNI_initModels(
    JNIEnv *env,
    jclass clazz,
    jstring ncnn_param_path,
    jstring ncnn_bin_path);

JNIEXPORT jobject JNICALL Java_com_yitu_paiwu_PaiwuJNI_getResult(
    JNIEnv *env,
    jclass clazz,
    jobject bitmap);

#ifdef __cplusplus
}
#endif

#endif // __ANDROID__

#endif // PAIWU_JNI_H
