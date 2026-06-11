//
// AntiDebug.h - Anti-debugging and anti-tracing protections
// Handles ptrace/TracerPid/frida/common debugger detection
//

#ifndef BLACKBOX_ANTIDEBUG_H
#define BLACKBOX_ANTIDEBUG_H

#include <jni.h>
#include <stdbool.h>

class AntiDebug {
public:
    // Install all anti-debug hooks. Call once during initialization.
    static void install(JNIEnv *env);

    // Enable/disable at runtime
    static void setEnabled(bool enabled);

    // Self-ptrace: attach to self to prevent external debuggers
    static void selfTrace();

    // Hook common debugger detection functions
    static void installDebugDetectionHooks(JNIEnv *env);

    // Frida detection mitigation
    static void installFridaMitigation();

private:
    static bool s_enabled;
};

// JNI exports for Java VMCore
extern "C" {
    void AntiDebug_install(JNIEnv *env, jclass clazz);
    void AntiDebug_selfTrace(JNIEnv *env, jclass clazz);
    void AntiDebug_setEnabled(JNIEnv *env, jclass clazz, jboolean enabled);
}

#endif //BLACKBOX_ANTIDEBUG_H