/*
 * rtlsdr433.cpp — JNI bridge between the Android app and the rtl_433 native decoder
 *
 * Provides JNI entry points called from NativeBridge.kt and routes decoded
 * output and device-state callbacks back to the Kotlin layer via static method
 * calls on NativeBridge.
 *
 * Copyright (C) 2025 Christian Ebner <info@ebctech.eu>
 *
 * This file is a derived work of rtl_433 (https://github.com/merbanan/rtl_433),
 * which is licensed under the GNU General Public License version 2 (GPL-2.0).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <jni.h>
#include <android/log.h>
#include <string.h>
#include <string>
#include <pthread.h>

// C headers must be wrapped in extern "C" to prevent C++ name mangling,
// which would cause linker failures when resolving symbols from the C translation units.
extern "C" {
#include "rtl_433.h"
#include "logger.h"
}

#define LOG_TAG "RTL433_JNI"

// JVM and NativeBridge class reference — used for callbacks from native threads.
// Protected by g_jni_mutex because initNative/releaseNative run on the main thread
// while announce_output_line/announce_device_stat run on the SDR thread.
static pthread_mutex_t g_jni_mutex = PTHREAD_MUTEX_INITIALIZER;
static JavaVM* g_javaVm  = nullptr;
static jint    g_javaVersion = 0;
static jclass  g_cls      = nullptr;

// ---------------------------------------------------------------------------
// rtl_433 log handler — routes print_logf() / print_log() messages to logcat
// ---------------------------------------------------------------------------

static void rtl433_logcat_handler(log_level_t level, char const *src, char const *msg, void *)
{
    android_LogPriority prio;
    switch (level) {
        case LOG_FATAL:    prio = ANDROID_LOG_FATAL;   break;
        case LOG_CRITICAL:
        case LOG_ERROR:    prio = ANDROID_LOG_ERROR;   break;
        case LOG_WARNING:  prio = ANDROID_LOG_WARN;    break;
        case LOG_NOTICE:
        case LOG_INFO:     prio = ANDROID_LOG_INFO;    break;
        case LOG_DEBUG:    prio = ANDROID_LOG_DEBUG;   break;
        default:           prio = ANDROID_LOG_VERBOSE; break;
    }
    __android_log_print(prio,  "RTL433", "%s", msg ? msg : "");
}

// Attach the calling thread to the JVM if needed, return env and whether to detach.
// Caller must NOT hold g_jni_mutex — this function only touches the JVM, not g_cls.
static bool attachThread(JNIEnv** env, bool* needDetach)
{
    *needDetach = false;
    if (!g_javaVm) return false;
    jint res = g_javaVm->GetEnv(reinterpret_cast<void**>(env), g_javaVersion);
    if (res == JNI_EDETACHED) {
        if (g_javaVm->AttachCurrentThread(env, nullptr) != 0) {
            __android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "AttachCurrentThread failed");
            return false;
        }
        *needDetach = true;
    }
    return *env != nullptr;
}

// ---------------------------------------------------------------------------
// C callbacks invoked from rtl_433 (android_bridge.c) and rtl_433.c
// ---------------------------------------------------------------------------

extern "C" void announce_output_line(const char *line)
{
    if (!line) return;

    pthread_mutex_lock(&g_jni_mutex);
    jclass cls = g_cls;  // snapshot under lock
    pthread_mutex_unlock(&g_jni_mutex);
    if (!cls) return;

    JNIEnv* env = nullptr;
    bool needDetach = false;
    if (!attachThread(&env, &needDetach)) return;

    jmethodID mid = env->GetStaticMethodID(cls, "nativeOutputLine", "(Ljava/lang/String;)V");
    if (!mid) {
        __android_log_write(ANDROID_LOG_ERROR, LOG_TAG,
                "announce_output_line: GetStaticMethodID for nativeOutputLine FAILED — check class/method name");
        if (env->ExceptionCheck()) { env->ExceptionDescribe(); env->ExceptionClear(); }
        if (needDetach) g_javaVm->DetachCurrentThread();
        return;
    }

    jstring jLine = env->NewStringUTF(line);
    if (jLine) {
        env->CallStaticVoidMethod(cls, mid, jLine);
        env->DeleteLocalRef(jLine);
    } else {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG,
                "announce_output_line: NewStringUTF failed (OOM?) for line len=%zu",
                strlen(line));
    }
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    if (needDetach) g_javaVm->DetachCurrentThread();
}

extern "C" void announce_device_stat(int devState)
{
    // Deduplicate: only log when state actually changes
    static int last_state = -1;
    if (devState != last_state) {
        static const char *state_names[] = {"STOPPED", "STARTING", "GRACE", "STARTED"};
        const char *name = (devState >= 0 && devState <= 3) ? state_names[devState] : "UNKNOWN";
        __android_log_print(ANDROID_LOG_INFO, "RTL433_STATE",
                "Device state: %s (%d)", name, devState);
        last_state = devState;
    }

    pthread_mutex_lock(&g_jni_mutex);
    jclass cls = g_cls;  // snapshot under lock
    pthread_mutex_unlock(&g_jni_mutex);
    if (!cls) return;

    JNIEnv* env = nullptr;
    bool needDetach = false;
    if (!attachThread(&env, &needDetach)) return;

    jmethodID mid = env->GetStaticMethodID(cls, "nativeDeviceStat", "(I)V");
    if (mid) {
        env->CallStaticVoidMethod(cls, mid, static_cast<jint>(devState));
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
    } else {
        if (env->ExceptionCheck()) { env->ExceptionDescribe(); env->ExceptionClear(); }
    }

    if (needDetach) g_javaVm->DetachCurrentThread();
}

// ---------------------------------------------------------------------------
// JNI entry points
// ---------------------------------------------------------------------------

extern "C"
JNIEXPORT jboolean JNICALL
Java_eu_ebctech_rtlsdr433andro_rtlsdr_NativeBridge_initNative(JNIEnv* env, jobject /*thiz*/)
{
    env->GetJavaVM(&g_javaVm);
    g_javaVersion = env->GetVersion();

    // Route all rtl_433 print_logf() / print_log() calls to Android logcat
    r_logger_set_log_handler(rtl433_logcat_handler, nullptr);
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG,
            "initNative: JNI version=0x%x, log handler registered", g_javaVersion);

    jclass local = env->FindClass("eu/ebctech/rtlsdr433andro/rtlsdr/NativeBridge");
    if (!local) {
        __android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "initNative: class not found");
        if (env->ExceptionCheck()) { env->ExceptionDescribe(); env->ExceptionClear(); }
        return JNI_FALSE;
    }
    jclass newGlobal = static_cast<jclass>(env->NewGlobalRef(local));
    env->DeleteLocalRef(local);

    if (!newGlobal) {
        __android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "initNative: NewGlobalRef failed");
        return JNI_FALSE;
    }

    // Swap under mutex: create new ref first, then delete old — prevents use-after-free
    pthread_mutex_lock(&g_jni_mutex);
    jclass oldCls = g_cls;
    g_cls = newGlobal;
    pthread_mutex_unlock(&g_jni_mutex);

    if (oldCls) {
        env->DeleteGlobalRef(oldCls);
    }

    __android_log_write(ANDROID_LOG_INFO, LOG_TAG, "initNative: OK");
    return JNI_TRUE;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_eu_ebctech_rtlsdr433andro_rtlsdr_NativeBridge_releaseNative(JNIEnv* env, jobject /*thiz*/)
{
    pthread_mutex_lock(&g_jni_mutex);
    jclass oldCls = g_cls;
    g_cls = nullptr;
    pthread_mutex_unlock(&g_jni_mutex);

    if (oldCls) {
        env->DeleteGlobalRef(oldCls);
    }
    __android_log_write(ANDROID_LOG_INFO, LOG_TAG, "releaseNative: global refs cleared");
    return JNI_TRUE;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_eu_ebctech_rtlsdr433andro_rtlsdr_NativeBridge_isNativeRunning(JNIEnv* /*env*/, jobject /*thiz*/)
{
    int running = rtl433_android_isrunning();
    // Sync device state to the UI whenever the running state is queried.
    // announce_device_stat() routes through nativeDeviceStat() → LiveJsonStore.setDevState()
    // so the StatusCard always reflects the correct hardware state.
    // We report STARTED(3) when the native loop is active, STOPPED(0) when not.
    // The intermediate STARTING(1) / GRACE(2) transitions are still pushed by the
    // native watchdog timer independently.
    announce_device_stat(running ? 3 : 0);
    return running ? JNI_TRUE : JNI_FALSE;
}

// start(fd, gainStr, frequencyHz, sampleRate, ppm, conversion, digitalAgc)
// NOTE: This function BLOCKS on the calling thread until the SDR loop exits.
extern "C"
JNIEXPORT void JNICALL
Java_eu_ebctech_rtlsdr433andro_rtlsdr_NativeBridge_start(
        JNIEnv* env, jobject /*thiz*/,
        jint fd,
        jstring gainStr,
        jint frequencyHz,
        jint sampleRate,
        jint ppm,
        jint conversion,
        jint digitalAgc)
{
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG,
            "JNI start() called: fd=%d freqHz=%d sampleRate=%d ppm=%d conv=%d digitalAgc=%d",
            (int)fd, (int)frequencyHz, (int)sampleRate, (int)ppm, (int)conversion, (int)digitalAgc);

    if ((int)fd <= 0) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,
                "JNI start() INVALID fd=%d — USB file descriptor missing!", (int)fd);
        return;
    }

    // fd
    android_usb_fd = static_cast<int>(fd);

    // gain string — empty means auto
    if (gainStr) {
        const char* gs = env->GetStringUTFChars(gainStr, nullptr);
        if (gs) {
            strncpy(android_gain_str, gs, sizeof(android_gain_str) - 1);
            android_gain_str[sizeof(android_gain_str) - 1] = '\0';
            env->ReleaseStringUTFChars(gainStr, gs);
        }
    } else {
        android_gain_str[0] = '\0';
    }

    android_frequency_hz = static_cast<int>(frequencyHz);
    android_sample_rate  = static_cast<int>(sampleRate);
    android_ppm          = static_cast<int>(ppm);
    android_conversion   = static_cast<int>(conversion);
    android_agc_mode     = static_cast<int>(digitalAgc);

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG,
            "JNI start() → rtl433_start() [blocking]: fd=%d gain='%s' freq=%d sr=%d ppm=%d conv=%d digitalAgc=%d",
            android_usb_fd, android_gain_str, android_frequency_hz,
            android_sample_rate, android_ppm, android_conversion, android_agc_mode);

    // BLOCKING — returns only when the SDR loop exits (via close() signal or error)
    // Re-register the logcat handler: r_free_cfg() clears it at the end of each run,
    // so it must be restored before every rtl433_start() call.
    r_logger_set_log_handler(rtl433_logcat_handler, nullptr);
    rtl433_start();

    __android_log_write(ANDROID_LOG_INFO, LOG_TAG, "JNI start() returned from rtl433_start()");
}

extern "C"
JNIEXPORT void JNICALL
Java_eu_ebctech_rtlsdr433andro_rtlsdr_NativeBridge_close(JNIEnv* /*env*/, jobject /*thiz*/)
{
    __android_log_write(ANDROID_LOG_INFO, LOG_TAG, "JNI close() → rtl433_android_close() (signal only)");
    rtl433_android_close();
}
