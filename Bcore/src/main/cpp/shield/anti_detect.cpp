//
// Created by refactor on 2026/06/01.
// Anti-detection module implementation.
//

#include "anti_detect.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <unordered_set>

#include "../utils/Log.h"
#include "../xhook/xhook.h"

#define AD_LOG_TAG "AntiDetect"
#define AD_LOGI(...) __android_log_print(ANDROID_LOG_INFO, AD_LOG_TAG, __VA_ARGS__)
#define AD_LOGW(...) __android_log_print(ANDROID_LOG_WARN, AD_LOG_TAG, __VA_ARGS__)
#define AD_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, AD_LOG_TAG, __VA_ARGS__)

#ifndef PR_SET_NAME
#define PR_SET_NAME 15
#endif

namespace shield {

volatile int AntiDetect::sInstalled = 0;

// 出现在 /proc/self/maps 行中即视为敏感,整行替换为空白
static const char* kSensitiveKeywords[] = {
    "blackdex",
    "shield",
    "BlackBox",
    "antide",
    nullptr,
};

// 用于 prctl 随机化进程名的"无害名"池
static const char* kProcessNamePool[] = {
    "system_server",
    "android.display",
    "webview_loader",
    "inputflinger",
    "logd",
    nullptr,
};

static const char* kMapsPath = "/proc/self/maps";
static const char* kStatusPath = "/proc/self/status";

struct SensitiveFileSet {
    pthread_mutex_t mutex;
    std::unordered_set<FILE*> files;
    SensitiveFileSet() : mutex(PTHREAD_MUTEX_INITIALIZER) {}
};
static SensitiveFileSet gSensitiveFiles;

static inline bool isSensitivePath(const char* path) {
    if (path == nullptr) return false;
    if (strcmp(path, kMapsPath) == 0) return true;
    if (strcmp(path, kStatusPath) == 0) return true;
    return false;
}

static inline void markSensitive(FILE* fp) {
    if (fp == nullptr) return;
    pthread_mutex_lock(&gSensitiveFiles.mutex);
    gSensitiveFiles.files.insert(fp);
    pthread_mutex_unlock(&gSensitiveFiles.mutex);
}

static inline bool isMarked(FILE* fp) {
    if (fp == nullptr) return false;
    pthread_mutex_lock(&gSensitiveFiles.mutex);
    bool hit = gSensitiveFiles.files.count(fp) > 0;
    pthread_mutex_unlock(&gSensitiveFiles.mutex);
    return hit;
}

static inline void unmarkSensitive(FILE* fp) {
    if (fp == nullptr) return;
    pthread_mutex_lock(&gSensitiveFiles.mutex);
    gSensitiveFiles.files.erase(fp);
    pthread_mutex_unlock(&gSensitiveFiles.mutex);
}

// ---------------------------------------------------------------------------
// xhook trampolines
// ---------------------------------------------------------------------------

AD_HOOK_FUN(FILE*, fopen, const char* path, const char* mode) {
    FILE* fp = orig_fopen(path, mode);
    if (fp != nullptr && isSensitivePath(path)) {
        markSensitive(fp);
    }
    return fp;
}

AD_HOOK_FUN(FILE*, fopen64, const char* path, const char* mode) {
    FILE* fp = orig_fopen64(path, mode);
    if (fp != nullptr && isSensitivePath(path)) {
        markSensitive(fp);
    }
    return fp;
}

AD_HOOK_FUN(int, fclose, FILE* stream) {
    if (stream != nullptr) unmarkSensitive(stream);
    return orig_fclose(stream);
}

static inline bool containsSensitive(const char* buf) {
    for (int i = 0; kSensitiveKeywords[i] != nullptr; ++i) {
        if (strstr(buf, kSensitiveKeywords[i]) != nullptr) return true;
    }
    return false;
}

static inline void sanitizeTracerPid(char* buf) {
    // 行格式: "TracerPid:\t<num>\n"
    char* p = buf + strlen("TracerPid:");
    while (*p == ' ' || *p == '\t') ++p;
    *p = '0';
    ++p;
    while (*p >= '0' && *p <= '9') {
        *p = ' ';
        ++p;
    }
}

AD_HOOK_FUN(char*, fgets, char* buf, int size, FILE* stream) {
    char* ret = orig_fgets(buf, size, stream);
    if (ret == nullptr) return nullptr;
    if (!isMarked(stream)) return ret;

    if (strncmp(buf, "TracerPid:", 10) == 0) {
        sanitizeTracerPid(buf);
    } else if (containsSensitive(buf)) {
        // 替换为空白行,保持换行结构
        buf[0] = '\n';
        buf[1] = '\0';
    }
    return ret;
}

// ---------------------------------------------------------------------------
// install helpers
// ---------------------------------------------------------------------------

int AntiDetect::installMapsHook() {
    const char* soName = ".*\\.so$";
    int rc = 0;
    if (xhook_register(soName, "fopen", (void*) new_fopen, (void**) &orig_fopen) != 0) {
        AD_LOGW("xhook_register fopen failed");
        rc |= 1;
    }
    if (xhook_register(soName, "fopen64", (void*) new_fopen64, (void**) &orig_fopen64) != 0) {
        AD_LOGW("xhook_register fopen64 failed");
        rc |= 2;
    }
    if (xhook_register(soName, "fgets", (void*) new_fgets, (void**) &orig_fgets) != 0) {
        AD_LOGW("xhook_register fgets failed");
        rc |= 4;
    }
    if (xhook_register(soName, "fclose", (void*) new_fclose, (void**) &orig_fclose) != 0) {
        AD_LOGW("xhook_register fclose failed");
        rc |= 8;
    }
    return rc;
}

int AntiDetect::installStatusHook() {
    // maps hook 已经覆盖 status(同走 fopen+fgets 路径)
    // 此处为占位,以便将来注入更细的 status 处理逻辑(如单行精确替换)
    return 0;
}

int AntiDetect::installNetworkHook() {
    // 端口隐藏:实际生产中需要更复杂的策略(例如 hook getifaddysockopt
    // 隐藏本地监听端口),此处只暴露一个 noop,后续迭代。
    AD_LOGI("network hook not yet implemented; reserved for future");
    return 0;
}

int AntiDetect::install() {
    if (sInstalled) {
        AD_LOGI("AntiDetect already installed");
        return 0;
    }
    int rc = 0;
    rc |= installMapsHook();
    rc |= installStatusHook();
    rc |= installNetworkHook();
    if (xhook_refresh(0) != 0) {
        AD_LOGE("xhook_refresh failed");
        rc |= 0x100;
    } else {
        AD_LOGI("xhook_refresh ok");
    }
    sInstalled = (rc == 0) ? 1 : -1;
    return rc;
}

void AntiDetect::uninstall() {
    if (!sInstalled) return;
    pthread_mutex_lock(&gSensitiveFiles.mutex);
    gSensitiveFiles.files.clear();
    pthread_mutex_unlock(&gSensitiveFiles.mutex);
    sInstalled = 0;
}

int AntiDetect::isInstalled() {
    return sInstalled;
}

int AntiDetect::randomizeProcessName() {
    unsigned int seed = (unsigned int) time(nullptr) ^ (unsigned int) getpid() ^
                        (unsigned int) syscall(__NR_gettid);
    int count = 0;
    while (kProcessNamePool[count] != nullptr) ++count;
    if (count == 0) return -1;
    const char* name = kProcessNamePool[seed % (unsigned int) count];
    if (prctl(PR_SET_NAME, (unsigned long) name, 0, 0, 0) != 0) {
        AD_LOGE("prctl PR_SET_NAME failed");
        return -1;
    }
    AD_LOGI("process name randomized to '%s'", name);
    return 0;
}

void AntiDetect::jniInstall(JNIEnv* env, jclass clazz) {
    (void) env;
    (void) clazz;
    install();
}

void AntiDetect::jniRandomizeProcessName(JNIEnv* env, jclass clazz) {
    (void) env;
    (void) clazz;
    randomizeProcessName();
}

}  // namespace shield
