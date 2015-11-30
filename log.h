#ifndef _LOG_H
#define _LOG_H

#ifdef ENABLE_DEBUG
#include <android/log.h>
#define LOG_TAG "ele7enxxh_inlineHook"
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG, fmt, ##args)
#else
#define LOGD(fmt,args...)
#endif

#endif