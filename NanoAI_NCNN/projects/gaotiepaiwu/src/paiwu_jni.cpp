#include "paiwu_jni.h"

#ifdef __ANDROID__

#include <android/bitmap.h>
#include <android/log.h>

#include <memory>
#include <vector>

#include <opencv2/opencv.hpp>

#include "nanoai_ncnn/projects/gaotiepaiwu/paiwu_detector.hpp"

namespace
{
    constexpr const char *kTag = "PaiwuJNI";

    std::unique_ptr<NanoAI_NCNN::Projects::GaoTiePaiWu::PaiwuDetector> g_paiwu_detector;

    jclass g_hash_map_cls = nullptr;
    jmethodID g_hash_map_ctor = nullptr;
    jmethodID g_hash_map_put = nullptr;

    jclass g_float_cls = nullptr;
    jmethodID g_float_ctor = nullptr;

    jclass g_integer_cls = nullptr;
    jmethodID g_integer_ctor = nullptr;

    bool bitmap_to_bgr_mat(JNIEnv *env, jobject bitmap, cv::Mat &bgr_mat)
    {
        if (bitmap == nullptr)
        {
            return false;
        }

        AndroidBitmapInfo bitmap_info{};
        if (AndroidBitmap_getInfo(env, bitmap, &bitmap_info) != ANDROID_BITMAP_RESULT_SUCCESS)
        {
            return false;
        }

        if (bitmap_info.format != ANDROID_BITMAP_FORMAT_RGBA_8888 &&
            bitmap_info.format != ANDROID_BITMAP_FORMAT_RGB_565)
        {
            return false;
        }

        void *pixels = nullptr;
        if (AndroidBitmap_lockPixels(env, bitmap, &pixels) != ANDROID_BITMAP_RESULT_SUCCESS || pixels == nullptr)
        {
            return false;
        }

        bool success = true;
        if (bitmap_info.format == ANDROID_BITMAP_FORMAT_RGBA_8888)
        {
            cv::Mat rgba(bitmap_info.height, bitmap_info.width, CV_8UC4, pixels);
            cv::cvtColor(rgba, bgr_mat, cv::COLOR_RGBA2BGR);
        }
        else if (bitmap_info.format == ANDROID_BITMAP_FORMAT_RGB_565)
        {
            cv::Mat rgb565(bitmap_info.height, bitmap_info.width, CV_8UC2, pixels);
            cv::cvtColor(rgb565, bgr_mat, cv::COLOR_BGR5652BGR);
        }
        else
        {
            success = false;
        }

        AndroidBitmap_unlockPixels(env, bitmap);
        return success && !bgr_mat.empty();
    }

    jobject create_array_list(JNIEnv *env, jmethodID &add_method)
    {
        jclass array_list_cls = env->FindClass("java/util/ArrayList");
        if (array_list_cls == nullptr)
        {
            return nullptr;
        }

        const jmethodID ctor = env->GetMethodID(array_list_cls, "<init>", "()V");
        add_method = env->GetMethodID(array_list_cls, "add", "(Ljava/lang/Object;)Z");
        if (ctor == nullptr || add_method == nullptr)
        {
            env->DeleteLocalRef(array_list_cls);
            return nullptr;
        }

        jobject list_obj = env->NewObject(array_list_cls, ctor);
        env->DeleteLocalRef(array_list_cls);
        return list_obj;
    }

    bool init_jni_ids(JNIEnv *env)
    {
        if (g_hash_map_cls != nullptr)
        {
            return true;
        }

        jclass local_hash_map_cls = env->FindClass("java/util/HashMap");
        jclass local_float_cls = env->FindClass("java/lang/Float");
        jclass local_integer_cls = env->FindClass("java/lang/Integer");

        if (local_hash_map_cls == nullptr || local_float_cls == nullptr || local_integer_cls == nullptr)
        {
            __android_log_print(ANDROID_LOG_ERROR, kTag, "FindClass failed for HashMap/Float/Integer");
            return false;
        }

        g_hash_map_cls = static_cast<jclass>(env->NewGlobalRef(local_hash_map_cls));
        g_float_cls = static_cast<jclass>(env->NewGlobalRef(local_float_cls));
        g_integer_cls = static_cast<jclass>(env->NewGlobalRef(local_integer_cls));

        env->DeleteLocalRef(local_hash_map_cls);
        env->DeleteLocalRef(local_float_cls);
        env->DeleteLocalRef(local_integer_cls);

        if (g_hash_map_cls == nullptr || g_float_cls == nullptr || g_integer_cls == nullptr)
        {
            return false;
        }

        g_hash_map_ctor = env->GetMethodID(g_hash_map_cls, "<init>", "()V");
        g_hash_map_put = env->GetMethodID(g_hash_map_cls, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

        g_float_ctor = env->GetMethodID(g_float_cls, "<init>", "(F)V");
        g_integer_ctor = env->GetMethodID(g_integer_cls, "<init>", "(I)V");

        return g_hash_map_ctor != nullptr &&
               g_hash_map_put != nullptr &&
               g_float_ctor != nullptr &&
               g_integer_ctor != nullptr;
    }

    void put_to_map(JNIEnv *env, jobject map_obj, const char *key, jobject value_obj)
    {
        jstring key_obj = env->NewStringUTF(key);
        if (key_obj == nullptr || value_obj == nullptr)
        {
            if (key_obj != nullptr)
            {
                env->DeleteLocalRef(key_obj);
            }
            return;
        }

        env->CallObjectMethod(map_obj, g_hash_map_put, key_obj, value_obj);
        env->DeleteLocalRef(key_obj);
    }
} // namespace

JNIEXPORT void JNICALL Java_com_yitu_paiwu_PaiwuJNI_initModels(
    JNIEnv *env,
    jclass,
    jstring ncnn_param_path,
    jstring ncnn_bin_path)
{
    if (g_paiwu_detector)
    {
        return;
    }

    if (ncnn_param_path == nullptr || ncnn_bin_path == nullptr)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "initModels failed: model path is null");
        return;
    }

    const char *param_path = env->GetStringUTFChars(ncnn_param_path, nullptr);
    const char *bin_path = env->GetStringUTFChars(ncnn_bin_path, nullptr);
    if (param_path == nullptr || bin_path == nullptr)
    {
        if (param_path != nullptr)
        {
            env->ReleaseStringUTFChars(ncnn_param_path, param_path);
        }
        if (bin_path != nullptr)
        {
            env->ReleaseStringUTFChars(ncnn_bin_path, bin_path);
        }

        __android_log_print(ANDROID_LOG_ERROR, kTag, "initModels failed: GetStringUTFChars returned null");
        return;
    }

    try
    {
        g_paiwu_detector = std::make_unique<NanoAI_NCNN::Projects::GaoTiePaiWu::PaiwuDetector>(
            param_path,
            bin_path);
    }
    catch (const std::exception &e)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "initModels exception: %s", e.what());
    }

    env->ReleaseStringUTFChars(ncnn_param_path, param_path);
    env->ReleaseStringUTFChars(ncnn_bin_path, bin_path);
}

JNIEXPORT jobject JNICALL Java_com_yitu_paiwu_PaiwuJNI_getResult(
    JNIEnv *env,
    jclass,
    jobject bitmap)
{
    jmethodID add_method = nullptr;
    jobject result_list = create_array_list(env, add_method);
    if (result_list == nullptr)
    {
        return nullptr;
    }

    if (!g_paiwu_detector || bitmap == nullptr)
    {
        return result_list;
    }

    if (!init_jni_ids(env))
    {
        return result_list;
    }

    cv::Mat input_image;
    if (!bitmap_to_bgr_mat(env, bitmap, input_image) || input_image.empty())
    {
        return result_list;
    }

    NanoAI_NCNN::Projects::GaoTiePaiWu::PaiwuDetections detections;
    g_paiwu_detector->detect(input_image, detections);

    for (const auto &det : detections)
    {
        jobject result_obj = env->NewObject(g_hash_map_cls, g_hash_map_ctor);
        if (result_obj == nullptr)
        {
            continue;
        }

        jobject left_obj = env->NewObject(g_float_cls, g_float_ctor, det.lx);
        jobject top_obj = env->NewObject(g_float_cls, g_float_ctor, det.ly);
        jobject right_obj = env->NewObject(g_float_cls, g_float_ctor, det.rx);
        jobject bottom_obj = env->NewObject(g_float_cls, g_float_ctor, det.ry);
        jobject class_id_obj = env->NewObject(g_integer_cls, g_integer_ctor, det.class_id);
        jstring class_name_obj = env->NewStringUTF(det.class_name.c_str());

        put_to_map(env, result_obj, "left", left_obj);
        put_to_map(env, result_obj, "top", top_obj);
        put_to_map(env, result_obj, "right", right_obj);
        put_to_map(env, result_obj, "bottom", bottom_obj);
        put_to_map(env, result_obj, "classId", class_id_obj);
        put_to_map(env, result_obj, "className", class_name_obj);

        env->DeleteLocalRef(left_obj);
        env->DeleteLocalRef(top_obj);
        env->DeleteLocalRef(right_obj);
        env->DeleteLocalRef(bottom_obj);
        env->DeleteLocalRef(class_id_obj);
        env->DeleteLocalRef(class_name_obj);

        env->CallBooleanMethod(result_list, add_method, result_obj);
        env->DeleteLocalRef(result_obj);
    }

    return result_list;
}

#endif // __ANDROID__
