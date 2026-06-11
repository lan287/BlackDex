//
// Created by Milk on 2021/5/16.
// Enhanced for mainstream packers support.
//

#ifndef BLACKBOX_DEXDUMP_H
#define BLACKBOX_DEXDUMP_H
#include <jni.h>
#include <string>
#include <vector>
#include <mutex>

#define HOOK_FUN(ret, func, ...) \
  ret (*orig_##func)(__VA_ARGS__); \
  ret new_##func(__VA_ARGS__)

class DexDump {
public:
    // Original dump entry points
    static void hookDumpDex(JNIEnv *env, jstring dir);
    static void cookieDumpDex(JNIEnv *env, jlong cookie, jstring dir, jboolean fix);

    // New: Enable all available dump hooks (recommended for modern packers)
    static void enableDeepDump(JNIEnv *env, jstring dir);

    // New: Dump via DexFile constructor interception (captures dex at load time)
    static void dumpByDexFileInit(JNIEnv *env, void *dex_file_begin, size_t dex_size);

    // New: Dump via ClassLinker::DefineClass (different code path for some packers)
    static void registerDefineClassHook(JNIEnv *env);

    // New: Enhanced dex extraction using internal class visitor
    static void dumpByClassVisitor(JNIEnv *env, void *klass);

    // Utility: Try to locate and dump dex data from a memory range
    static void scanAndDumpDex(JNIEnv *env, const void *start, size_t length);

private:
    static bool verifyAndFixDexHeader(void *buffer, size_t size);
    static void safeDumpDex(JNIEnv *env, void *begin, size_t size);
public:
    static bool writeDexFile(const char *dir, void *data, size_t size, const char *prefix);
};

#endif //BLACKBOX_DEXDUMP_H