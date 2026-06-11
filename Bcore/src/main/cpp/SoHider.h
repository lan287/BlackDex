//
// Created by blackbox on 2024/5/20.
// Enhanced: hide runtime artifacts from all common detection vectors.
// See SoHider.h for design notes.
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

    // New: Filter /proc/self/smaps content (same API as readMapsFiltered)
    static char *readSmapsFiltered(size_t *length);

    // New: Filter /proc/self/fd content (hide symlinks to hidden SOs)
    static char *readFdDirFiltered(size_t *length);

    // New: Add a path pattern to hide (for advanced use)
    static void addHiddenPath(const char *pathSubstring);

    // New: Protect linker namespace from enumeration
    // (hooks dl_iterate_phdr to filter out hidden libraries)
    static void installLinkerNamespaceHook();

private:
    static bool isHidden(const char *path);
};

#endif //VIRTUALM_SOHIDER_H