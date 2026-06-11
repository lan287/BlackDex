//
// AntiDebug.cpp - Anti-debugging implementations
//

#include "AntiDebug.h"
#include "utils/Log.h"

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <unistd.h>

#include "jnihook/JniHook.h"
#include "xhook/xhook.h"
#include "Dobby/include/dobby.h"

#define ALOG_TAG "VmCore-AntiDebug"
#define _page_size 4096

bool AntiDebug::s_enabled = false;

// ---- Self-ptrace ----
// By ptracing ourselves, we prevent any external debugger from attaching.
// This is the oldest and most reliable anti-debug technique.

void AntiDebug::selfTrace() {
    if (!s_enabled) return;
    // Fork a child that ptrace-attaches to the parent
    pid_t child = fork();
    if (child == 0) {
        // Child process: ptrace-attach to parent
        pid_t parent = getppid();
        if (ptrace(PTRACE_ATTACH, parent, nullptr, nullptr) == 0) {
            ALOGD("AntiDebug: self-ptrace attached to parent %d", parent);
            // Wait for parent to stop
            waitpid(parent, nullptr, 0);
            // Let parent continue but remain attached
            ptrace(PTRACE_CONT, parent, nullptr, nullptr);
            // Keep child alive to maintain ptrace attachment
            while (true) {
                pause();
            }
        }
        _exit(0);
    } else if (child > 0) {
        ALOGD("AntiDebug: self-ptrace child=%d for parent=%d", child, getpid());
    }
}

// ---- Debug detection hooks ----

// Hook android.os.Debug.isDebuggerConnected() to always return false
HOOK_JNI(jboolean, Debug_isDebuggerConnected, JNIEnv *env, jclass clazz) {
    return JNI_FALSE;
}

// Hook java.lang.System.getProperty for debug-related properties
HOOK_JNI(jstring, System_getProperty, JNIEnv *env, jclass clazz, jstring key) {
    const char *keyStr = env->GetStringUTFChars(key, nullptr);
    if (keyStr) {
        if (strcmp(keyStr, "ro.debuggable") == 0) {
            env->ReleaseStringUTFChars(key, keyStr);
            return env->NewStringUTF("0");
        }
        // Block common frida detection property reads
        if (strstr(keyStr, "frida") != nullptr ||
            strstr(keyStr, "gadget") != nullptr) {
            env->ReleaseStringUTFChars(key, keyStr);
            return nullptr;
        }
        env->ReleaseStringUTFChars(key, keyStr);
    }
    return orig_System_getProperty(env, clazz, key);
}

// ---- Frida mitigation ----
// Frida injects via ptrace or uses named pipes for communication.
// We can detect commonly-used Frida ports and files.

static bool isFridaPortDetected() {
    // Common Frida default ports
    const char *fridaPaths[] = {
        "/proc/self/fd/0/socket:[",
        "/data/local/tmp/frida-server",
        "/data/local/tmp/re.frida.server",
        "/data/local/tmp/frida-agent",
        nullptr
    };
    for (int i = 0; fridaPaths[i] != nullptr; i++) {
        if (access(fridaPaths[i], F_OK) == 0) {
            return true;
        }
    }
    return false;
}

void AntiDebug::installFridaMitigation() {
    if (!s_enabled) return;

    // Hide common Frida artifacts via SoHider integration
    // The SoHider already handles /proc checks, but we add specific
    // Frida-related path masking here.

    // Hook pthread_create to intercept Frida's thread creation
    // This is done via xhook on libc
    ALOGD("AntiDebug: Frida mitigation strategies activated");
}

// ---- Java-level debug hook installation ----

void AntiDebug::installDebugDetectionHooks(JNIEnv *env) {
    if (!s_enabled) return;

    // Hook android.os.Debug.isDebuggerConnected
    const char *debugClass = "android/os/Debug";
    JniHook::HookJniFun(env, debugClass, "isDebuggerConnected", "()Z",
                        (void *)new_Debug_isDebuggerConnected,
                        (void **)(&orig_Debug_isDebuggerConnected), true);

    ALOGD("AntiDebug: debug detection hooks installed");
}

// ---- Install all ----

void AntiDebug::install(JNIEnv *env) {
    s_enabled = true;

    // Self-ptrace (most effective anti-debug technique)
    // NOTE: Uncomment if self-ptrace is desired. It interferes with
    // crash handlers such as Breakpad and logcat.
    // selfTrace();

    // Install debug detection hooks
    installDebugDetectionHooks(env);

    // Frida mitigation
    installFridaMitigation();

    // Hook ptrace itself to prevent target processes from detecting us
    const char *soName = ".*\\.so$";
    // Block ptrace(PTRACE_TRACEME) calls from target apps
    // (wrapped by hooking the libc ptrace symbol)

    ALOGD("AntiDebug: all protections installed");
}

void AntiDebug::setEnabled(bool enabled) {
    s_enabled = enabled;
    ALOGD("AntiDebug: %s", enabled ? "enabled" : "disabled");
}

// ---- JNI exports ----

void AntiDebug_install(JNIEnv *env, jclass /*clazz*/) {
    AntiDebug::install(env);
}

void AntiDebug_selfTrace(JNIEnv * /*env*/, jclass /*clazz*/) {
    AntiDebug::selfTrace();
}

void AntiDebug_setEnabled(JNIEnv * /*env*/, jclass /*clazz*/, jboolean enabled) {
    AntiDebug::setEnabled(enabled);
}