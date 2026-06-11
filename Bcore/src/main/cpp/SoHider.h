//
// Created by blackbox on 2024/5/20.
// Hide runtime artifacts so protected apps cannot detect us via:
//   * dlopen / android_dlopen_ext / do_dlopen  (loader)
//   * open / openat / fopen / access / stat     (fs)
//   * /proc/self/maps / /proc/self/task/.../maps (memory layout)
//
// The hider is conservative: only artifacts that we ourselves loaded are
// suppressed. We never modify behaviour for unrelated libraries.
//

#ifndef VIRTUALM_SOHIDER_H
#define VIRTUALM_SOHIDER_H

#include <jni.h>
#include <stddef.h>

class SoHider {
public:
    // Enable all hooks. Safe to call multiple times (idempotent).
    static void init(JNIEnv *env);

    // Toggle at runtime. Useful for tests / benchmarks.
    static void setEnabled(bool enabled);

    // Register a new basename that should be hidden (e.g. an extra *.so shipped
    // by the host application).
    static void addHiddenName(const char *basename);

    // Read a filtered copy of /proc/self/maps. The caller owns the returned
    // buffer and must free() it. *length is set to the size of the buffer in
    // bytes (excluding the trailing NUL).
    static char *readMapsFiltered(size_t *length);
};

#endif //VIRTUALM_SOHIDER_H
