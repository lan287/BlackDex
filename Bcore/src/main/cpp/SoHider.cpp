//
// Created by blackbox on 2024/5/20.
// See SoHider.h for design notes.
//

#define _GNU_SOURCE

#include "SoHider.h"
#include "utils/Log.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <link.h>
#include <mutex>
#include <string>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#include "xhook/xhook.h"

#define ALOG_TAG "VmCore-Hide"

// memfd_create wrapper using syscall for compatibility with API < 30
#include <sys/syscall.h>
static inline int memfd_create_compat(const char *name, unsigned int flags) {
#ifdef __NR_memfd_create
    return syscall(__NR_memfd_create, name, flags);
#else
    return -1;
#endif
}

// Forward declarations for link-time resolution
extern "C" void* dlopen(const char*, int) __attribute__((weak));
extern "C" void* android_dlopen_ext(const char*, int, const void*) __attribute__((weak));
extern "C" void* do_dlopen(const char*, int, const void*) __attribute__((weak));

namespace {

// We *intentionally* match a small, well-known set of basenames. Anything
// that matches one of these is something we either own or a third-party hook
// library that the host has explicit reason to load. We do NOT do substring
// matching: too many false positives in real apps.
static const char *kDefaultHidden[] = {
    "libblackdex.so",
    "libblackdex_d.so",
    "libdobby.so",
    "libdobby_d.so",
    "libdexdumper.so",
    "libdexdumper_d.so",
    "libdexdump.so",
    "libdexdump_d.so",
};

// Names that mark detection probes (not our own artifacts) so we do not
// actually suppress their dlopen attempts; we just feed them the failure path
// silently.
std::vector<std::string> g_hidden_names;
std::mutex g_hidden_mutex;
std::atomic<bool> g_enabled{false};
std::atomic<bool> g_in_hook{false};

bool basenameIsHidden(const char *path) {
    if (path == nullptr) return false;
    const char *base = strrchr(path, '/');
    base = (base == nullptr) ? path : base + 1;
    {
        std::lock_guard<std::mutex> lock(g_hidden_mutex);
        for (const auto &n : g_hidden_names) {
            if (strcmp(base, n.c_str()) == 0) return true;
        }
    }
    return false;
}

bool pathIsHidden(const char *path) {
    if (path == nullptr) return false;
    // Match either exact basename or any segment that *contains* the
    // basename: "/data/.../libblackdex.so" etc.
    if (basenameIsHidden(path)) return true;
    return strstr(path, "libblackdex") != nullptr ||
           strstr(path, "libdobby") != nullptr;
}

// ---- dlopen replacement --------------------------------------------------
// We swallow the request rather than returning a stub handle: most detectors
// pass the returned handle to dlsym, which would crash on a fake handle.

void *new_dlopen(const char *filename, int flag) {
    if (g_enabled.load(std::memory_order_acquire) && pathIsHidden(filename)) {
        ALOGD("dlopen blocked: %s", filename == nullptr ? "(null)" : filename);
        errno = ENOENT;
        return nullptr;
    }
    // Use the original symbol so the loader does the real work.
    return dlopen(filename, flag);
}

void *new_android_dlopen_ext(const char *filename, int flag, const void *extinfo) {
    if (g_enabled.load(std::memory_order_acquire) && pathIsHidden(filename)) {
        ALOGD("android_dlopen_ext blocked: %s", filename == nullptr ? "(null)" : filename);
        errno = ENOENT;
        return nullptr;
    }
    return android_dlopen_ext(filename, flag, extinfo);
}

void *new_do_dlopen(const char *filename, int flag, const void *extinfo) {
    if (g_enabled.load(std::memory_order_acquire) && pathIsHidden(filename)) {
        ALOGD("do_dlopen blocked: %s", filename == nullptr ? "(null)" : filename);
        errno = ENOENT;
        return nullptr;
    }
    return do_dlopen(filename, flag, extinfo);
}

// ---- file access replacement --------------------------------------------
// We cannot fully hide an SO from the filesystem: target app needs to load
// it. We can, however, prevent the target from reading /proc/self/maps and
// from probing for the SO via open().

// open() is a varargs function in the libc signature but glibc actually has
// two entry points: open and openat. Both must be hooked.
int new_open(const char *pathname, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = static_cast<mode_t>(va_arg(ap, int));
        va_end(ap);
    }
    if (g_enabled.load(std::memory_order_acquire) && pathname != nullptr) {
        // /proc/self/maps + per-task maps: feed them through a redacted view.
        if (strcmp(pathname, "/proc/self/maps") == 0) {
            size_t len = 0;
            char *filtered = SoHider::readMapsFiltered(&len);
            if (filtered == nullptr) {
                // Could not read; pretend it is empty so the probe sees nothing.
                int tmp = memfd_create_compat("empty", 0);
                return tmp >= 0 ? tmp : -1;
            }
            int fd = memfd_create_compat("maps", 0);
            if (fd >= 0) {
                // ftruncate is required by some kernel versions for the size hint.
                ftruncate(fd, static_cast<off_t>(len));
                write(fd, filtered, len);
                lseek(fd, 0, SEEK_SET);
            }
            free(filtered);
            return fd;
        }
    }
    extern int open(const char *, int, ...);
    return ::open(pathname, flags, mode);
}

int new_openat(int dirfd, const char *pathname, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = static_cast<mode_t>(va_arg(ap, int));
        va_end(ap);
    }
    if (g_enabled.load(std::memory_order_acquire) && pathname != nullptr) {
        if (strcmp(pathname, "/proc/self/maps") == 0 ||
            strcmp(pathname, "/proc/self/task/../maps") == 0) {
            return new_open("/proc/self/maps", O_RDONLY);
        }
    }
    extern int openat(int, const char *, int, ...);
    return ::openat(dirfd, pathname, flags, mode);
}

int new_access(const char *pathname, int mode) {
    if (g_enabled.load(std::memory_order_acquire) && pathIsHidden(pathname)) {
        ALOGD("access blocked: %s", pathname);
        errno = ENOENT;
        return -1;
    }
    extern int access(const char *, int);
    return ::access(pathname, mode);
}

int new_stat(const char *pathname, struct stat *buf) {
    if (g_enabled.load(std::memory_order_acquire) && pathIsHidden(pathname)) {
        ALOGD("stat blocked: %s", pathname);
        errno = ENOENT;
        return -1;
    }
    extern int stat(const char *, struct stat *);
    return ::stat(pathname, buf);
}

int new_fstatat(int dirfd, const char *pathname, struct stat *buf, int flags) {
    if (g_enabled.load(std::memory_order_acquire) && pathIsHidden(pathname)) {
        ALOGD("fstatat blocked: %s", pathname);
        errno = ENOENT;
        return -1;
    }
    extern int fstatat(int, const char *, struct stat *, int);
    return ::fstatat(dirfd, pathname, buf, flags);
}

}  // namespace

// Public API ---------------------------------------------------------------

void SoHider::init(JNIEnv * /*env*/) {
    bool expected = false;
    if (!g_enabled.compare_exchange_strong(expected, true)) {
        return;  // already initialised
    }
    {
        std::lock_guard<std::mutex> lock(g_hidden_mutex);
        g_hidden_names.reserve(sizeof(kDefaultHidden) / sizeof(kDefaultHidden[0]));
        for (const char *n : kDefaultHidden) {
            g_hidden_names.emplace_back(n);
        }
    }
    // Match any *.so loaded by this process.
    const char *soName = ".*\\.so$";

    // dlopen family
    xhook_register(soName, "dlopen",
                   reinterpret_cast<void *>(new_dlopen),
                   nullptr);
    xhook_register(soName, "android_dlopen_ext",
                   reinterpret_cast<void *>(new_android_dlopen_ext),
                   nullptr);
    xhook_register(soName, "do_dlopen",
                   reinterpret_cast<void *>(new_do_dlopen),
                   nullptr);

    // file access family
    xhook_register(soName, "open",
                   reinterpret_cast<void *>(new_open),
                   nullptr);
    xhook_register(soName, "openat",
                   reinterpret_cast<void *>(new_openat),
                   nullptr);
    xhook_register(soName, "access",
                   reinterpret_cast<void *>(new_access),
                   nullptr);
    xhook_register(soName, "stat",
                   reinterpret_cast<void *>(new_stat),
                   nullptr);
    xhook_register(soName, "fstatat",
                   reinterpret_cast<void *>(new_fstatat),
                   nullptr);

    xhook_refresh(0);
    ALOGD("SoHider initialised");
}

void SoHider::setEnabled(bool enabled) {
    g_enabled.store(enabled, std::memory_order_release);
    ALOGD("SoHider %s", enabled ? "enabled" : "disabled");
}

void SoHider::addHiddenName(const char *basename) {
    if (basename == nullptr) return;
    std::lock_guard<std::mutex> lock(g_hidden_mutex);
    std::string n(basename);
    if (std::find(g_hidden_names.begin(), g_hidden_names.end(), n) == g_hidden_names.end()) {
        g_hidden_names.emplace_back(std::move(n));
    }
}

namespace {

bool lineIsHidden(const char *line) {
    // /proc/self/maps line: address perms offset dev inode pathname
    const char *path = strchr(line, '/');
    if (path == nullptr) return false;
    if (pathIsHidden(path)) return true;
    // [vdso]/[vsyscall] lines never have a "/" so the above is enough.
    return false;
}

}  // namespace

char *SoHider::readMapsFiltered(size_t *length) {
    if (length != nullptr) *length = 0;
    FILE *f = fopen("/proc/self/maps", "r");
    if (f == nullptr) return nullptr;

    const size_t kCap = 256 * 1024;
    size_t cap = 4096;
    size_t used = 0;
    char *buf = static_cast<char *>(malloc(cap));
    if (buf == nullptr) {
        fclose(f);
        return nullptr;
    }

    char *line = nullptr;
    size_t capline = 0;
    ssize_t n;
    while ((n = getline(&line, &capline, f)) >= 0) {
        if (lineIsHidden(line)) continue;
        if (used + static_cast<size_t>(n) + 1 > cap) {
            size_t newcap = cap;
            while (newcap < used + static_cast<size_t>(n) + 1) newcap *= 2;
            if (newcap > kCap) {
                // Refuse to allocate a giant buffer: bail out with what we have.
                break;
            }
            char *nb = static_cast<char *>(realloc(buf, newcap));
            if (nb == nullptr) break;
            buf = nb;
            cap = newcap;
        }
        memcpy(buf + used, line, static_cast<size_t>(n));
        used += static_cast<size_t>(n);
    }
    free(line);
    fclose(f);

    if (buf == nullptr) return nullptr;
    buf[used] = '\0';
    if (length != nullptr) *length = used;
    return buf;
}
