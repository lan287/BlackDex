//
// Created by Milk on 2021/5/16.
// Enhanced for mainstream packers (360, Tencent Legu, Bangcle, etc.)
//

#include "DexDump.h"
#include "utils/Log.h"
#include "VmCore.h"
#include "utils/PointerCheck.h"
#include "jnihook/Art.h"
#include "jnihook/ArtM.h"

#include <cstdio>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <set>
#include <mutex>
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>

#include "dex/dex_file-inl.h"
#include "dex/dex_file_loader.h"
#include "jnihook/JniHook.h"
#include "xhook/xhook.h"
#include "Dobby/include/dobby.h"

#define _uintval(p)               reinterpret_cast<uintptr_t>(p)
#define _ptr(p)                   reinterpret_cast<void *>(p)
#define _align_up(x, n)           (((x) + ((n) - 1)) & ~((n) - 1))
#define _align_down(x, n)         ((x) & -(n))
#define _page_size                4096
#define _page_align(n)            _align_up(static_cast<uintptr_t>(n), _page_size)
#define _ptr_align(x)             _ptr(_align_down(reinterpret_cast<uintptr_t>(x), _page_size))
#define _make_rwx(p, n)           ::mprotect(_ptr_align(p), \
                                              _page_align(_uintval(p) + n) != _page_align(_uintval(p)) ? _page_align(n) + _page_size : _page_align(n), \
                                              PROT_READ | PROT_WRITE | PROT_EXEC)

using namespace std;

static int beginOffset = -2;
static const char *hookDumpPath = nullptr;
static std::set<size_t> dumpedDexSizes;
static std::mutex dumpMutex;

// ---- Dex header verification & repair ----
static const uint8_t kDexMagic[] = {0x64, 0x65, 0x78, 0x0a, 0x30, 0x33, 0x35, 0x00};
static const uint8_t kDexMagicSize = 8;

// Scan backward from an address to find a valid dex magic header
static void *findDexMagicBefore(void *ptr, size_t searchRange) {
    auto *p = static_cast<uint8_t *>(ptr);
    for (size_t i = 0; i < searchRange; i++) {
        if (memcmp(p - i, kDexMagic, kDexMagicSize) == 0) {
            return p - i;
        }
    }
    return nullptr;
}

// Scan forward for dex magic
static void *findDexMagicInRange(void *start, size_t range) {
    auto *p = static_cast<uint8_t *>(start);
    for (size_t i = 0; i + kDexMagicSize <= range; i++) {
        if (memcmp(p + i, kDexMagic, kDexMagicSize) == 0) {
            return p + i;
        }
    }
    return nullptr;
}

// Fix dex header: magic, checksum, signature, file_size
static void fixDexHeaderFields(void *buffer, size_t actualSize) {
    auto *bytes = static_cast<uint8_t *>(buffer);
    // Fix magic
    memcpy(bytes, kDexMagic, kDexMagicSize);

    // Fix file_size at offset 32 (little-endian)
    uint32_t sizeLE = static_cast<uint32_t>(actualSize);
    memcpy(bytes + 32, &sizeLE, 4);

    // Fix checksum at offset 8 (Adler32 of data after checksum field)
    uint32_t adler = 1;
    uint32_t s2 = 0;
    static const uint32_t MOD_ADLER = 65521;
    for (size_t i = 12; i < actualSize; i++) {
        adler = (adler + bytes[i]) % MOD_ADLER;
        s2 = (s2 + adler) % MOD_ADLER;
    }
    uint32_t checksum = (s2 << 16) | adler;
    memcpy(bytes + 8, &checksum, 4);

    // Fix SHA-1 signature at offset 12 (20 bytes after checksum)
    memset(bytes + 12, 0, 20);
}

bool DexDump::verifyAndFixDexHeader(void *buffer, size_t size) {
    if (size < 0x70) return false;
    auto *bytes = static_cast<uint8_t *>(buffer);

    bool magicValid = (memcmp(bytes, kDexMagic, kDexMagicSize) == 0);
    uint32_t declaredSize = *reinterpret_cast<uint32_t *>(bytes + 32);
    bool sizeValid = (declaredSize > 0 && declaredSize <= size + 0x1000);

    if (!magicValid || !sizeValid) {
        fixDexHeaderFields(buffer, size);
        return true;
    }
    return false;
}

// ---- Dex file writing ----

bool DexDump::writeDexFile(const char *dir, void *data, size_t size, const char *prefix) {
    if (size < 0x70 || !PointerCheck::check(data)) return false;

    {
        lock_guard<mutex> lock(dumpMutex);
        if (dumpedDexSizes.find(size) != dumpedDexSizes.end()) {
            return false;
        }
        dumpedDexSizes.insert(size);
    }

    void *buffer = malloc(size);
    if (!buffer) return false;
    memcpy(buffer, data, size);

    verifyAndFixDexHeader(buffer, size);

    const bool kVerifyChecksum = false;
    const bool kVerify = false;
    const art_lkchan::DexFileLoader dex_file_loader;
    std::string error_msg;
    std::vector<std::unique_ptr<const art_lkchan::DexFile>> dex_files;
    if (!dex_file_loader.OpenAll(reinterpret_cast<const uint8_t *>(buffer),
                                 size, "", kVerify, kVerifyChecksum, &error_msg, &dex_files)) {
        ALOGD("DexDump: open dex warning: %s (size=%zu), writing anyway", error_msg.c_str(), size);
    }

    char path[1024];
    snprintf(path, sizeof(path), "%s/%s_%zu.dex", dir, prefix, size);
    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0600);
    if (fd < 0) {
        free(buffer);
        return false;
    }
    ssize_t w = write(fd, buffer, size);
    fsync(fd);
    close(fd);

    if (w > 0) {
        ALOGD("DexDump: %s dump success => %s (%zu bytes)", prefix, path, size);
        free(buffer);
        return true;
    }
    remove(path);
    free(buffer);
    return false;
}

void DexDump::safeDumpDex(JNIEnv *env, void *begin, size_t size) {
    if (!hookDumpPath || !begin || size < 0x70) return;
    DexDump::writeDexFile(hookDumpPath, begin, size, "deep");
}

// ---- Original handleDumpByDexFile (kept for backward compatibility) ----

static void handleDumpByDexFile(void *dex_file) {
    if (!PointerCheck::check(dex_file)) return;
    auto dexFile = static_cast<art_lkchan::DexFile *>(dex_file);
    int size = dexFile->Size();
    {
        lock_guard<mutex> lock(dumpMutex);
        if (dumpedDexSizes.find(size) != dumpedDexSizes.end()) return;
        dumpedDexSizes.insert(size);
    }

    void *buffer = malloc(size);
    if (!buffer) return;
    memcpy(buffer, dexFile->Begin(), size);
    fixDexHeaderFields(buffer, size);

    char path[1024];
    snprintf(path, sizeof(path), "%s/hook_%d.dex", hookDumpPath, size);
    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0600);
    ssize_t w = write(fd, buffer, size);
    fsync(fd);
    close(fd);
    if (w > 0) {
        ALOGD("DexDump: hook dump => %s", path);
    } else {
        remove(path);
    }
    free(buffer);
}

// ---- Anti-kill hooks ----

HOOK_JNI(int, kill, pid_t __pid, int __signal) {
    ALOGE("DexDump: hooked kill, blocking");
    return 0;
}

HOOK_JNI(int, killpg, int __pgrp, int __signal) {
    ALOGE("DexDump: hooked killpg, blocking");
    return 0;
}

// ---- LoadMethod hooks (original approach) ----

HOOK_FUN(void, LoadMethodO, void *thiz,
         void *dex_file, void *method, void *klass, void *dst) {
    orig_LoadMethodO(thiz, dex_file, method, klass, dst);
    handleDumpByDexFile(dex_file);
}

HOOK_FUN(void, LoadMethodM, void *thiz,
         void *thread, void *dex_file, void *method, void *klass, void *dst) {
    orig_LoadMethodM(thiz, thread, dex_file, method, klass, dst);
    handleDumpByDexFile(dex_file);
}

HOOK_FUN(void, LoadMethodL, void *thiz,
         void *thread, void *dex_file, void *method, void *klass) {
    orig_LoadMethodL(thiz, thread, dex_file, method, klass);
    handleDumpByDexFile(dex_file);
}

// ===================================================================
// Deep dump hooks for modern packers
// ===================================================================

// ---- Hook: DexFile::DexFile constructor ----
HOOK_FUN(void, DexFileCtor, void *thiz, const uint8_t *base, size_t size,
         const std::string &location, uint32_t location_checksum,
         void *oat_dex_file, bool is_compact_dex) {
    orig_DexFileCtor(thiz, base, size, location, location_checksum, oat_dex_file, is_compact_dex);
    if (base && size > 0x70 && hookDumpPath) {
        DexDump::writeDexFile(hookDumpPath, const_cast<uint8_t *>(base), size, "ctor");
    }
}

// ---- Hook: ClassLinker::DefineClass ----
HOOK_FUN(void, DefineClass, void *thiz, void *thread, const char *descriptor,
         size_t hash, void *class_loader, const void *dex_file,
         const void *class_def) {
    orig_DefineClass(thiz, thread, descriptor, hash, class_loader, dex_file, class_def);
    if (hookDumpPath) {
        handleDumpByDexFile(const_cast<void *>(dex_file));
    }
}

// ---- Hook: DexFileLoader::OpenCommon ----
static void *(*orig_OpenCommon)(const uint8_t *base, size_t size,
                                 const std::string &location, uint32_t location_checksum,
                                 void *oat_dex_file, bool verify, bool verify_checksum,
                                 std::string *error_msg, std::unique_ptr<const void> *out_dex_file) = nullptr;

static void *new_OpenCommon(const uint8_t *base, size_t size,
                             const std::string &location, uint32_t location_checksum,
                             void *oat_dex_file, bool verify, bool verify_checksum,
                             std::string *error_msg, std::unique_ptr<const void> *out_dex_file) {
    if (base && size > 0x70 && hookDumpPath) {
        DexDump::writeDexFile(hookDumpPath, const_cast<uint8_t *>(base), size, "open");
    }
    return orig_OpenCommon(base, size, location, location_checksum,
                           oat_dex_file, verify, verify_checksum, error_msg, out_dex_file);
}

// ---- Hook: InMemoryDexClassLoader constructor (Android 8+) ----
HOOK_JNI(jobject, InMemoryDexClassLoader_init, JNIEnv *env, jobject thiz,
         jobject buffer, jobject class_loader) {
    if (buffer && hookDumpPath) {
        void *addr = env->GetDirectBufferAddress(buffer);
        jlong cap = env->GetDirectBufferCapacity(buffer);
        if (addr && cap > 0x70) {
            DexDump::writeDexFile(hookDumpPath, addr, static_cast<size_t>(cap), "imcl");
        }
    }
    return orig_InMemoryDexClassLoader_init(env, thiz, buffer, class_loader);
}

// ---- Scan memory for orphaned dex files ----
void DexDump::scanAndDumpDex(JNIEnv *env, const void *start, size_t length) {
    if (!hookDumpPath || !start || length < 0x70) return;
    const auto *mem = static_cast<const uint8_t *>(start);
    size_t scanned = 0;

    while (scanned + kDexMagicSize <= length) {
        if (memcmp(mem + scanned, kDexMagic, kDexMagicSize) == 0) {
            const uint8_t *dexStart = mem + scanned;
            if (scanned + 0x70 <= length) {
                uint32_t dexSize = *reinterpret_cast<const uint32_t *>(dexStart + 32);
                if (dexSize > 0x70 && dexSize < 100 * 1024 * 1024 && scanned + dexSize <= length) {
                    DexDump::writeDexFile(hookDumpPath, const_cast<uint8_t *>(dexStart), dexSize, "scan");
                    scanned += dexSize;
                    continue;
                }
            }
            size_t remaining = length - scanned;
            size_t dumpSize = (remaining > 50 * 1024 * 1024) ? 50 * 1024 * 1024 : remaining;
            DexDump::writeDexFile(hookDumpPath, const_cast<uint8_t *>(dexStart), dumpSize, "scan_partial");
            scanned += dumpSize;
        } else {
            scanned++;
        }
    }
}

// ---- Hook registration helpers ----

static void registerOpenCommonHook() {
    const char *libart = "libart.so";
    void *openCommon = DobbySymbolResolver(libart,
        "_ZN3art14DexFileLoader10OpenCommonEPKhjRKNSt6__ndk112basic_stringIcNS2_11char_traitsIcEENS2_9allocatorIcEEEEjPNS_10OatDexFileEbbPS9_");
    if (!openCommon) {
        openCommon = DobbySymbolResolver(libart,
            "_ZN3art14DexFileLoader10OpenCommonEPKhmRKNSt6__ndk112basic_stringIcNS3_11char_traitsIcEENS3_9allocatorIcEEEEjPNS_10OatDexFileEbbPSA_");
    }
    if (openCommon) {
        _make_rwx(openCommon, _page_size);
        DobbyHook(openCommon, (void *)new_OpenCommon, (void **)&orig_OpenCommon);
        ALOGD("DexDump: OpenCommon hook installed");
    }
}

static void registerInMemoryDexClassLoaderHook(JNIEnv *env) {
    if (android_get_device_api_level() < __ANDROID_API_O__) return;
    const char *className = "dalvik/system/InMemoryDexClassLoader";
    jclass imcl = env->FindClass(className);
    if (!imcl) {
        env->ExceptionClear();
        return;
    }
    JniHook::HookJniFun(env, className, "<init>",
                        "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V",
                        (void *)new_InMemoryDexClassLoader_init,
                        (void **)(&orig_InMemoryDexClassLoader_init), false);
    ALOGD("DexDump: InMemoryDexClassLoader hook installed");
}

static void registerDexFileCtorHook() {
    const char *libart = "libart.so";
    void *dexFileCtor = DobbySymbolResolver(libart,
        "_ZN3art7DexFileC1EPKhmRKNSt6__ndk112basic_stringIcNS2_11char_traitsIcEENS2_9allocatorIcEEEEjPNS_10OatDexFileEb");
    if (!dexFileCtor) {
        dexFileCtor = DobbySymbolResolver(libart,
            "_ZN3art7DexFileC1EPKhjRKNSt6__ndk112basic_stringIcNS2_11char_traitsIcEENS2_9allocatorIcEEEEjPNS_10OatDexFileEb");
    }
    if (!dexFileCtor) {
        dexFileCtor = DobbySymbolResolver(libart,
            "_ZN3art7DexFileC1EPKhmRKNSt3__112basic_stringIcNS3_11char_traitsIcEENS3_9allocatorIcEEEEjPNS_10OatDexFileEb");
    }
    if (dexFileCtor) {
        _make_rwx(dexFileCtor, _page_size);
        DobbyHook(dexFileCtor, (void *)new_DexFileCtor, (void **)&orig_DexFileCtor);
        ALOGD("DexDump: DexFileCtor hook installed");
    }
}

static void registerDefineClassHookStatic() {
    const char *libart = "libart.so";
    void *defineClass = DobbySymbolResolver(libart,
        "_ZN3art11ClassLinker11DefineClassEPNS_6ThreadEPKcmNS_6HandleINS_6mirror11ClassLoaderEEERKNS_7DexFileERKNS_10ClassDefE");
    if (!defineClass) {
        defineClass = DobbySymbolResolver(libart,
            "_ZN3art11ClassLinker11DefineClassEPNS_6ThreadEPKcjNS_6HandleINS_6mirror11ClassLoaderEEERKNS_7DexFileERKNS_10ClassDefE");
    }
    if (!defineClass) {
        defineClass = DobbySymbolResolver(libart,
            "_ZN3art11ClassLinker11DefineClassEPNS_6ThreadEPKcmNS_6HandleINS_6mirror11ClassLoaderEEERKNS_7DexFileERKNS_10ClassDefENS_17ClassDataItemIteratorE");
    }
    if (defineClass) {
        _make_rwx(defineClass, _page_size);
        DobbyHook(defineClass, (void *)new_DefineClass, (void **)&orig_DefineClass);
        ALOGD("DexDump: DefineClass hook installed");
    }
}

// ===================================================================
// Original init / cookieDumpDex / hookDumpDex
// ===================================================================

static void init(JNIEnv *env) {
    const char *soName = ".*\\.so$";
    xhook_register(soName, "kill", (void *) new_kill, (void **) (&orig_kill));
    xhook_register(soName, "killpg", (void *) new_killpg, (void **) (&orig_killpg));
    xhook_refresh(0);

    jlongArray emptyCookie = VmCore::loadEmptyDex(env);
    if (!emptyCookie) {
        ALOGD("DexDump: loadEmptyDex returned null");
        beginOffset = -1;
        return;
    }
    jsize arrSize = env->GetArrayLength(emptyCookie);
    if (env->ExceptionCheck() == JNI_TRUE) {
        env->ExceptionClear();
        ALOGD("DexDump: Exception after GetArrayLength");
        beginOffset = -1;
        return;
    }
    if (arrSize == 0) {
        ALOGD("DexDump: empty cookie array (size=0)");
        beginOffset = -1;
        return;
    }
    jlong *long_data = env->GetLongArrayElements(emptyCookie, nullptr);
    if (!long_data) {
        ALOGD("DexDump: GetLongArrayElements returned null");
        beginOffset = -1;
        return;
    }

    for (int i = 0; i < arrSize; ++i) {
        jlong cookie = long_data[i];
        if (cookie == 0) continue;
        auto dex = reinterpret_cast<char *>(cookie);
        // Scan more offsets for newer Android versions
        int maxOffset = (android_get_device_api_level() >= __ANDROID_API_R__) ? 20 : 10;
        for (int ii = 0; ii < maxOffset; ++ii) {
            auto value = *(size_t *) (dex + ii * sizeof(size_t));
            if (value == 1872) {
                beginOffset = ii - 1;
                ALOGD("DexDump: cookie init success, beginOffset=%d, api=%d", beginOffset, android_get_device_api_level());
                env->ReleaseLongArrayElements(emptyCookie, long_data, 0);
                return;
            }
        }
        // Fallback: try to find a valid pointer by checking if any slot looks like a pointer
        // to a valid dex header (starts with dex\n magic)
        for (int ii = 0; ii < maxOffset; ++ii) {
            auto candidate = *(size_t *) (dex + ii * sizeof(size_t));
            if (candidate > 0x1000 && candidate < 0x7fffffffffff) {
                auto *candidatePtr = reinterpret_cast<uint8_t *>(candidate);
                if (memcmp(candidatePtr, kDexMagic, kDexMagicSize) == 0) {
                    beginOffset = ii;
                    ALOGD("DexDump: cookie init via dex magic fallback, beginOffset=%d", beginOffset);
                    env->ReleaseLongArrayElements(emptyCookie, long_data, 0);
                    return;
                }
            }
        }
    }
    env->ReleaseLongArrayElements(emptyCookie, long_data, 0);
    ALOGD("DexDump: cookie init failed, dump not supported on this device (api=%d)", android_get_device_api_level());
    beginOffset = -1;
}

// ---- CodeItem fixer ----

static void fixCodeItem(JNIEnv *env, const art_lkchan::DexFile *dex_file_, size_t begin) {
    for (size_t classdef_ctr = 0; classdef_ctr < dex_file_->NumClassDefs(); ++classdef_ctr) {
        const art_lkchan::DexFile::ClassDef &cd = dex_file_->GetClassDef(classdef_ctr);
        const uint8_t *class_data = dex_file_->GetClassData(cd);
        auto &classTypeId = dex_file_->GetTypeId(cd.class_idx_);
        std::string class_name = dex_file_->GetTypeDescriptor(classTypeId);

        if (class_data != nullptr) {
            art_lkchan::ClassDataItemIterator cdit(*dex_file_, class_data);
            cdit.SkipAllFields();
            while (cdit.HasNextMethod()) {
                const art_lkchan::DexFile::MethodId &method_id_item = dex_file_->GetMethodId(
                        cdit.GetMemberIndex());
                auto method_name = dex_file_->GetMethodName(method_id_item);
                auto method_signature = dex_file_->GetMethodSignature(
                        method_id_item).ToString().c_str();
                auto java_method = VmCore::findMethod(env, class_name.c_str(), method_name,
                                                      method_signature);
                if (java_method) {
                    auto artMethod = ArtM::GetArtMethod(env, java_method);
                    const art_lkchan::DexFile::CodeItem *orig_code_item = cdit.GetMethodCodeItem();
                    if (cdit.GetMethodCodeItemOffset() && orig_code_item) {
                        auto codeItemSize = dex_file_->GetCodeItemSize(*orig_code_item);
                        auto new_code_item = begin + ArtM::GetArtMethodDexCodeItemOffset(artMethod);
                        memcpy((void *) orig_code_item, (void *) new_code_item, codeItemSize);
                    }
                } else {
                    env->ExceptionClear();
                }
                cdit.Next();
            }
        }
    }
}

// ---- cookieDumpDex (cookie-based dump) ----

void DexDump::cookieDumpDex(JNIEnv *env, jlong cookie, jstring dir, jboolean fix) {
    if (beginOffset == -2) init(env);
    if (beginOffset == -1) {
        ALOGD("DexDump: cookie dump not supported on this device!");
        return;
    }
    auto base = reinterpret_cast<char *>(cookie);
    auto begin = *(size_t *) (base + beginOffset * sizeof(size_t));
    if (!PointerCheck::check(reinterpret_cast<void *>(begin))) return;

    auto dirC = env->GetStringUTFChars(dir, nullptr);

    size_t size = 0;
    auto beginPtr = reinterpret_cast<uintptr_t>(begin);

    uint32_t sizeFromHeader = *reinterpret_cast<uint32_t *>(beginPtr + 0x20);
    uint32_t sizeFromAlt = *reinterpret_cast<uint32_t *>(beginPtr + 0x24);
    uint32_t sizeFromAlt2 = *reinterpret_cast<uint32_t *>(beginPtr + 0x28);

    if (sizeFromHeader > 0x70 && sizeFromHeader < 100 * 1024 * 1024) {
        size = sizeFromHeader;
    } else if (sizeFromAlt > 0x70 && sizeFromAlt < 100 * 1024 * 1024) {
        size = sizeFromAlt;
    } else if (sizeFromAlt2 > 0x70 && sizeFromAlt2 < 100 * 1024 * 1024) {
        size = sizeFromAlt2;
    } else {
        void *magicPos = findDexMagicInRange(reinterpret_cast<void *>(begin), 0x1000);
        if (magicPos) {
            auto offset = static_cast<size_t>(static_cast<uint8_t *>(magicPos) - static_cast<uint8_t *>(reinterpret_cast<void *>(begin)));
            if (offset > 0 && offset < 0x1000) {
                size = offset + 0x70;
                uint32_t sz = *reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(magicPos) + 0x20);
                if (sz > 0x70 && sz < 100 * 1024 * 1024) size = offset + sz;
            }
        }
        if (size == 0) size = 0x500000;
    }

    void *buffer = malloc(size);
    if (!buffer) {
        env->ReleaseStringUTFChars(dir, dirC);
        return;
    }
    memset(buffer, 0, size);
    size_t copySize = (size > 100 * 1024 * 1024) ? 100 * 1024 * 1024 : size;
    memcpy(buffer, reinterpret_cast<const void *>(begin), copySize);

    fixDexHeaderFields(buffer, copySize);

    if (fix) {
        const bool kVerifyChecksum = false;
        const bool kVerify = false;
        const art_lkchan::DexFileLoader dex_file_loader;
        std::string error_msg;
        std::vector<std::unique_ptr<const art_lkchan::DexFile>> dex_files;
        if (dex_file_loader.OpenAll(reinterpret_cast<const uint8_t *>(buffer),
                                    copySize, "", kVerify, kVerifyChecksum, &error_msg, &dex_files)) {
            if (!dex_files.empty()) {
                fixCodeItem(env, dex_files[0].get(), begin);
            }
        }
    }

    char path[1024];
    snprintf(path, sizeof(path), "%s/cookie_%zu.dex", dirC, copySize);
    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0600);
    ssize_t w = write(fd, buffer, copySize);
    fsync(fd);
    close(fd);
    if (w > 0) {
        ALOGD("DexDump: cookie dump => %s", path);
    } else {
        remove(path);
    }
    free(buffer);
    env->ReleaseStringUTFChars(dir, dirC);
}

// ---- hookDumpDex (LoadMethod-based) ----

void DexDump::hookDumpDex(JNIEnv *env, jstring dir) {
    hookDumpPath = env->GetStringUTFChars(dir, nullptr);
    const char *libart = "libart.so";

    void *loadMethod = DobbySymbolResolver(libart,
       "_ZN3art11ClassLinker10LoadMethodEPNS_6ThreadERKNS_7DexFileERKNS_21ClassDataItemIteratorENS_6HandleINS_6mirror5ClassEEE");
    if (!loadMethod) {
        loadMethod = DobbySymbolResolver(libart,
           "_ZN3art11ClassLinker10LoadMethodEPNS_6ThreadERKNS_7DexFileERKNS_21ClassDataItemIteratorENS_6HandleINS_6mirror5ClassEEEPNS_9ArtMethodE");
    }
    if (!loadMethod) {
        loadMethod = DobbySymbolResolver(libart,
           "_ZN3art11ClassLinker10LoadMethodERKNS_7DexFileERKNS_13ClassAccessor6MethodENS_6HandleINS_6mirror5ClassEEEPNS_9ArtMethodE");
    }

    if (loadMethod) {
        _make_rwx(loadMethod, _page_size);
        if (android_get_device_api_level() >= __ANDROID_API_O__) {
            DobbyHook(loadMethod, (void *) new_LoadMethodO, (void **) &orig_LoadMethodO);
        } else if (android_get_device_api_level() >= __ANDROID_API_M__) {
            DobbyHook(loadMethod, (void *) new_LoadMethodM, (void **) &orig_LoadMethodM);
        } else {
            DobbyHook(loadMethod, (void *) new_LoadMethodL, (void **) &orig_LoadMethodL);
        }
        ALOGD("DexDump: LoadMethod hook installed");
    }

    // Also install deep dump hooks for modern packers
    registerOpenCommonHook();
    registerDexFileCtorHook();
    registerDefineClassHookStatic();
    registerInMemoryDexClassLoaderHook(env);
}

// ===================================================================
// enableDeepDump - Comprehensive multi-point dump
// ===================================================================

void DexDump::enableDeepDump(JNIEnv *env, jstring dir) {
    hookDumpPath = env->GetStringUTFChars(dir, nullptr);

    // Install anti-kill
    const char *soName = ".*\\.so$";
    xhook_register(soName, "kill", (void *) new_kill, (void **) (&orig_kill));
    xhook_register(soName, "killpg", (void *) new_killpg, (void **) (&orig_killpg));
    xhook_refresh(0);

    // Install all deep dump hooks
    registerOpenCommonHook();
    registerDexFileCtorHook();
    registerDefineClassHookStatic();
    registerInMemoryDexClassLoaderHook(env);

    // Also install LoadMethod hook for backward compatibility
    const char *libart = "libart.so";
    void *loadMethod = DobbySymbolResolver(libart,
       "_ZN3art11ClassLinker10LoadMethodERKNS_7DexFileERKNS_13ClassAccessor6MethodENS_6HandleINS_6mirror5ClassEEEPNS_9ArtMethodE");
    if (!loadMethod) {
        loadMethod = DobbySymbolResolver(libart,
           "_ZN3art11ClassLinker10LoadMethodEPNS_6ThreadERKNS_7DexFileERKNS_21ClassDataItemIteratorENS_6HandleINS_6mirror5ClassEEEPNS_9ArtMethodE");
    }
    if (!loadMethod) {
        loadMethod = DobbySymbolResolver(libart,
           "_ZN3art11ClassLinker10LoadMethodEPNS_6ThreadERKNS_7DexFileERKNS_21ClassDataItemIteratorENS_6HandleINS_6mirror5ClassEEE");
    }
    if (loadMethod) {
        _make_rwx(loadMethod, _page_size);
        int api = android_get_device_api_level();
        if (api >= __ANDROID_API_O__) {
            DobbyHook(loadMethod, (void *) new_LoadMethodO, (void **) &orig_LoadMethodO);
        } else if (api >= __ANDROID_API_M__) {
            DobbyHook(loadMethod, (void *) new_LoadMethodM, (void **) &orig_LoadMethodM);
        } else {
            DobbyHook(loadMethod, (void *) new_LoadMethodL, (void **) &orig_LoadMethodL);
        }
        ALOGD("DexDump: LoadMethod hook installed (deep mode)");
    }

    ALOGD("DexDump: Deep dump mode enabled with %zu hooks",
          (loadMethod ? 1 : 0) + 3);
}

// ---- Stub implementations for future expansion ----

void DexDump::dumpByDexFileInit(JNIEnv *env, void *dex_file_begin, size_t dex_size) {
    if (hookDumpPath) {
        DexDump::writeDexFile(hookDumpPath, dex_file_begin, dex_size, "dexfile");
    }
}

void DexDump::registerDefineClassHook(JNIEnv *env) {
    registerDefineClassHookStatic();
}

void DexDump::dumpByClassVisitor(JNIEnv *env, void *klass) {
    (void)env;
    (void)klass;
}