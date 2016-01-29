/**
 * Created @Zhouxm on 11/27/15.
 * 12/15/15 @Zhouxm : add MY_LOG_TAG
 */

#ifndef ANDROIDJNI_LIB_LOGGER_H
#define ANDROIDJNI_LIB_LOGGER_H

#include <android/log.h>
#define TAG "jni_log"

/**
 * 定义log标签
 */

#define MY_LOG_LEVEL_DEFAULT ANDROID_LOG_VERBOSE  //1
#define MY_LOG_LEVEL_VERBOSE ANDROID_LOG_VERBOSE  //2
#define MY_LOG_LEVEL_DEBUG ANDROID_LOG_DEBUG //3
#define MY_LOG_LEVEL_INFO ANDROID_LOG_INFO	//4
#define MY_LOG_LEVEL_WARNIG ANDROID_LOG_WARING	//5
#define MY_LOG_LEVEL_ERROR ANDROID_LOG_ERROR //6
#define MY_LOG_LEVEL_FATAL ANDROID_LOG_FATAL //7
#define MY_LOG_LEVEL_SILENT ANDROID_LOG_SILENT //8

#define MY_LOG_NOOP (void)(0)

#ifndef MY_LOG_TAG
#	define MY_LOG_TAG __FILE__
#endif

#ifndef MY_LOG_TAG
#	define MY_LOG_LEVEL MY_LOG_LEVEL_VERBOSE
#endif

#define MY_LOG_PRINT(level, fmt, ...) __android_log_print(level, MY_LOG_TAG, fmt,  ##__VA_ARGS__)

/**
 * 定义verbose信息
 */
#if ( MY_LOG_LEVEL_VERBOSE >= MY_LOG_LEVEL)
#	define LOGV(fmt, ...) MY_LOG_PRINT(ANDROID_LOG_VERBOSE, fmt, ##__VA_ARGS__)
#else
//#	define LOGV(...) MY_LOG_NOOP
#	define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, ##__VA_ARGS__)
#endif

/**
 * 定义debug信息
 */

#if ( MY_LOG_LEVEL_DEBUG >= MY_LOG_LEVEL)
#	define LOGD(fmt, ...) MY_LOG_PRINT(ANDROID_LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
//#	define LOGD(...) MY_LOG_NOOP
#	define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, ##__VA_ARGS__)
#endif

/**
 * 定义info信息
 */
#if ( MY_LOG_LEVEL_INFO >= MY_LOG_LEVEL)
#	define LOGI(fmt, ...) MY_LOG_PRINT(ANDROID_LOG_INFO, fmt, ##__VA_ARGS__)
#else
//#	define LOGI(...) MY_LOG_NOOP
#	define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, ##__VA_ARGS__)
#endif

/**
 * 定义warning信息
 */
#if ( MY_LOG_LEVEL_WARNIG >= MY_LOG_LEVEL)
#	define LOGW(fmt, ...) MY_LOG_PRINT(ANDROID_LOG_WARING, fmt, ##__VA_ARGS__)
#else
//#	define LOGW(...) MY_LOG_NOOP
#	define LOGW(...) __android_log_print(ANDROID_LOG_WARNING, TAG, ##__VA_ARGS__)
#endif

/**
 * 定义error信息
 */
#if ( MY_LOG_LEVEL_ERROR >= MY_LOG_LEVEL)
#	define LOGE(fmt, ...) MY_LOG_PRINT(ANDROID_LOG_ERROR, fmt, ##__VA_ARGS__)
#else
//#	define LOGE(...) MY_LOG_NOOP
#	define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, ##__VA_ARGS__)
#endif

/**
 * 定义fatal信息
 */
#if ( MY_LOG_LEVEL_FATAL >= MY_LOG_LEVEL)
#	define LOGF(fmt, ...) MY_LOG_PRINT(ANDROID_LOG_FATAL, fmt, ##__VA_ARGS__)
#else
//#	define LOGF(...) MY_LOG_NOOP
#	define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, TAG, ##__VA_ARGS__)
#endif

/**
 * 定义silent信息
 */
#if ( MY_LOG_LEVEL_SILENT >= MY_LOG_LEVEL)
#	define LOGS(fmt, ...) MY_LOG_PRINT(ANDROID_LOG_SILENT, fmt, ##__VA_ARGS__)
#else
//#	define LOGS(...) MY_LOG_NOOP
#	define LOGS(...) __android_log_print(ANDROID_LOG_SILENT, TAG, ##__VA_ARGS__)
#endif



#endif //ANDROIDJNI_LIB_LOGGER_H
