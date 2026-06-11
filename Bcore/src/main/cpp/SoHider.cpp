//
// Created by blackbox on 2024/5/20.
// Enhanced: comprehensive artifact hiding for modern anti-detection.
//
// Detection vectors now covered:
//   - dlopen / android_dlopen_ext / do_dlopen          (loader)
//   - open / openat / fopen / open64                   (file open)
//   - access / faccessat                                (existence check)
//   - stat / fstatat / lstat / stat64 / lstat64        (file metadata)
//   - readlink / readlinkat                             (symlink resolution)
//   - /proc/self/maps / /proc/self/smaps                (memory layout)
//   - /proc/self/fd/*                                   (file descriptor probing)
//   - /proc/self/status (TracerPid)                    (debugger detection)
//   - dl_iterate_phdr                                   (linker namespace walk)
//

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
#include <dirent.h>
#include <fcntl.h>
#include <link.h>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "xhook/xhook.h"
#include "Dobby/include/dobby.h"

#define ALOG_TAG "VmCore-Hide"

namespace {

// We *intentionally* match a small, well-known set of basenames. Anything
// that matches one of these is something we either own or a third-party hook
// library that the host has explicit reason to load.
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

// Additional path substrings to hide (for SOs in non-standard locations)
static const char *kDefaultHiddenPaths[] = {
    // Common framework library paths
    "/data/data/",  // covers app-private SO paths for hidden libs
};

std::vector<std::string> g_hidden_names;
std::vector<std::string> g_hidden_paths;
std::mutex g_hidden_mutex;
std::atomic<bool> g_enabled{false};
std::atomic<bool> g_linker_hook_installed{false};

// ---- Path matching ----

bool SoHider::isHidden(const char *path) {
    if (path == nullptr) return false;

    // Check basename
    const char *base = strrchr(path, '/');
    base = (base == nullptr) ? path : base + 1;
    {
        std::lock_guard<std::mutex> lock(g_hidden_mutex);
        for (const auto &n : g_hidden_names) {
            if (strcmp(base, n.c_str()) == 0) return true;
        }
        // Check path substrings
        for (const auto &p : g_hidden_paths) {
            if (strstr(path, p.c_str()) != nullptr) {
                // Only match if the basename is also in the hidden list
                for (const auto &n : g_hidden_names) {
                    if (strcmp(base, n.c_str()) == 0) return true;
                }
            }
        }
    }
    return false;
}

bool basenameIsHidden(const char *path) {
    return SoHider::isHidden(path);
}

bool pathIsHidden(const char *path) {
    if (path == nullptr) return false;
    if (basenameIsHidden(path)) return true;
    // Also check for raw substring match (legacy)
    return strstr(path, "libblackdex") != nullptr ||
           strstr(path, "libdobby") != nullptr;
}

// ---- dlopen family replacements ----

void *new_dlopen(const char *filename, int flag) {
    if (g_enabled.load(std::memory_order_acquire) && pathIsHidden(filename)) {
        ALOGD("dlopen blocked: %s", filename == nullptr ? "(null)" : filename);
        errno = ENOENT;
        return nullptr;
    }
    extern void *dlopen(const char *, int);
    return ::dlopen(filename, flag);
}

void *new_android_dlopen_ext(const char *filename, int flag, const void *extinfo) {
    if (g_enabled.load(std::memory_order_acquire) && pathIsHidden(filename)) {
        ALOGD("android_dlopen_ext blocked: %s", filename == nullptr ? "(null)" : filename);
        errno = ENOENT;
        return nullptr;
    }
    extern void *android_dlopen_ext(const char *, int, const void *);
    return ::android_dlopen_ext(filename, flag, extinfo);
}

void *new_do_dlopen(const char *filename, int flag, const void *extinfo) {
    if (g_enabled.load(std::memory_order_acquire) && pathIsHidden(filename)) {
        ALOGD("do_dlopen blocked: %s", filename == nullptr ? "(null)" : filename);
        errno = ENOENT;
        return nullptr;
    }
    extern void *do_dlopen(const char *, int, const void *);
    return ::do_dlopen(filename, flag, extinfo);
}

// ---- fopen family ----

FILE *new_fopen(const char *pathname, const char *mode) {
    if (g_enabled.load(std::memory_order_acquire) && pathname != nullptr) {
        if (strcmp(pathname, "/proc/self/maps") == 0 ||
            strcmp(pathname, "/proc/self/smaps") == 0 ||
            strstr(pathname, "/proc/self/task/") != nullptr) {
            // Handle /proc files specially - return a memfd-based filtered view
            size_t len = 0;
            char *filtered = (strstr(pathname, "smaps") != nullptr)
                ? SoHider::readSmapsFiltered(&len)
                : SoHider::readMapsFiltered(&len);
            if (filtered == nullptr) {
                errno = ENOENT;
                return nullptr;
            }
            // Create a temporary file with filtered content
            int fd = memfd_create("filtered_maps", 0);
            if (fd >= 0) {
                write(fd, filtered, len);
                lseek(fd, 0, SEEK_SET);
                free(filtered);
                FILE *fp = fdopen(fd, mode);
                if (fp != nullptr) return fp;
                close(fd);
            }
            free(filtered);
            errno = ENOENT;
            return nullptr;
        }
        if (pathIsHidden(pathname)) {
            ALOGD("fopen blocked: %s", pathname);
            errno = ENOENT;
            return nullptr;
        }
    }
    extern FILE *fopen(const char *, const char *);
    return ::fopen(pathname, mode);
}

FILE *new_fopen64(const char *pathname, const char *mode) {
    return new_fopen(pathname, mode);
}

// ---- readlink family ----
// /proc/self/fd/N points to hidden libraries. This hides those symlinks.

ssize_t new_readlink(const char *pathname, char *buf, size_t bufsize) {
    if (g_enabled.load(std::memory_order_acquire) && pathname != nullptr) {
        // If reading a /proc/self/fd link that points to a hidden SO, hide it
        if (strstr(pathname, "/proc/") == pathname && strstr(pathname, "/fd/") != nullptr) {
            extern ssize_t readlink(const char *, char *, size_t);
            ssize_t ret = ::readlink(pathname, buf, bufsize);
            if (ret > 0 && pathIsHidden(buf)) {
                // Return ENOENT as if the fd doesn't exist
                ALOGD("readlink hidden: %s -> %s", pathname, buf);
                errno = ENOENT;
                return -1;
            }
            return ret;
        }
    }
    extern ssize_t readlink(const char *, char *, size_t);
    return ::readlink(pathname, buf, bufsize);
}

ssize_t new_readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsize) {
    if (g_enabled.load(std::memory_order_acquire) && pathname != nullptr) {
        extern ssize_t readlinkat(int, const char *, char *, size_t);
        ssize_t ret = ::readlinkat(dirfd, pathname, buf, bufsize);
        if (ret > 0 && pathIsHidden(buf)) {
            ALOGD("readlinkat hidden: %s -> %s", pathname, buf);
            errno = ENOENT;
            return -1;
        }
        return ret;
    }
    extern ssize_t readlinkat(int, const char *, char *, size_t);
    return ::readlinkat(dirfd, pathname, buf, bufsize);
}

// ---- lstat (symlink-safe stat) ----

int new_lstat(const char *pathname, struct stat *buf) {
    if (g_enabled.load(std::memory_order_acquire) && pathIsHidden(pathname)) {
        ALOGD("lstat blocked: %s", pathname);
        errno = ENOENT;
        return -1;
    }
    // Special handling for /proc/self/fd directory
    if (pathname != nullptr && strstr(pathname, "/proc/") == pathname &&
        strstr(pathname, "/fd") != nullptr) {
        // Check if this fd points to a hidden SO
        char linktarget[PATH_MAX];
        ssize_t len = ::readlink(pathname, linktarget, sizeof(linktarget) - 1);
        if (len > 0) {
            linktarget[len] = '\0';
            if (pathIsHidden(linktarget)) {
                ALOGD("lstat fd hidden: %s -> %s", pathname, linktarget);
                errno = ENOENT;
                return -1;
            }
        }
    }
    extern int lstat(const char *, struct stat *);
    return ::lstat(pathname, buf);
}

// ---- stat64 ----

int new_stat64(const char *pathname, struct stat64 *buf) {
    if (g_enabled.load(std::memory_order_acquire) && pathIsHidden(pathname)) {
        ALOGD("stat64 blocked: %s", pathname);
        errno = ENOENT;
        return -1;
    }
    extern int stat64(const char *, struct stat64 *);
    return ::stat64(pathname, buf);
}

int new_lstat64(const char *pathname, struct stat64 *buf) {
    if (g_enabled.load(std::memory_order_acquire) && pathIsHidden(pathname)) {
        ALOGD("lstat64 blocked: %s", pathname);
        errno = ENOENT;
        return -1;
    }
    extern int lstat64(const char *, struct stat64 *);
    return ::lstat64(pathname, buf);
}

// ---- file access replacements (open/openat/access/stat/fstatat) ----

int new_open(const char *pathname, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = static_cast<mode_t>(va_arg(ap, int));
        va_end(ap);
    }
    if (g_enabled.load(std::memory_order_acquire) && pathname != nullptr) {
        // /proc/self/maps and /proc/self/smaps: filtered view
        if (strcmp(pathname, "/proc/self/maps") == 0 ||
            strcmp(pathname, "/proc/self/smaps") == 0) {
            size_t len = 0;
            char *filtered = (strcmp(pathname, "/proc/self/smaps") == 0)
                ? SoHider::readSmapsFiltered(&len)
                : SoHider::readMapsFiltered(&len);
            if (filtered == nullptr) {
                int tmp = memfd_create("empty", 0);
                return tmp >= 0 ? tmp : -1;
            }
            int fd = memfd_create("maps", 0);
            if (fd >= 0) {
                ftruncate(fd, static_cast<off_t>(len));
                write(fd, filtered, len);
                lseek(fd, 0, SEEK_SET);
            }
            free(filtered);
            return fd;
        }
        // /proc/self/status: hide TracerPid
        if (strcmp(pathname, "/proc/self/status") == 0) {
            // Read original and redact TracerPid line
            extern int open(const char *, int, ...);
            int realFd = ::open(pathname, O_RDONLY);
            if (realFd < 0) return realFd;

            // Read entire file
            char statusBuf[8192];
            ssize_t nRead = read(realFd, statusBuf, sizeof(statusBuf) - 1);
            close(realFd);
            if (nRead <= 0) {
                int tmp = memfd_create("status", 0);
                return tmp >= 0 ? tmp : -1;
            }
            statusBuf[nRead] = '\0';

            // Replace TracerPid value with 0
            std::string filtered(statusBuf);
            size_t pos = 0;
            while ((pos = filtered.find("TracerPid:", pos)) != std::string::npos) {
                size_t tabPos = filtered.find('\t', pos);
                size_t newlinePos = filtered.find('\n', pos);
                if (tabPos != std::string::npos && newlinePos != std::string::npos &&
                    tabPos < newlinePos) {
                    // Replace value between tab and newline with "0"
                    filtered.replace(tabPos + 1, newlinePos - tabPos - 1, "0");
                }
                pos = newlinePos + 1;
            }

            int fd = memfd_create("status", 0);
            if (fd >= 0) {
                write(fd, filtered.c_str(), filtered.size());
                lseek(fd, 0, SEEK_SET);
            }
            return fd;
        }
        // /proc/self/fd: filtered directory listing
        if (strstr(pathname, "/proc/") == pathname &&
            strstr(pathname, "/fd") != nullptr) {
            // Block direct /proc fd probing
            // Return fd pointing to an empty filtered view
            ALOGD("open fd probing blocked: %s", pathname);
            int fd = memfd_create("empty_fd", 0);
            return fd >= 0 ? fd : -1;
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
            strcmp(pathname, "/proc/self/smaps") == 0 ||
            strcmp(pathname, "/proc/self/status") == 0 ||
            strstr(pathname, "/proc/self/task/") != nullptr) {
            return new_open(pathname, O_RDONLY);
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
    // Also hide /proc/self/fd access to hidden SOs
    if (pathname != nullptr && strstr(pathname, "/proc/") == pathname &&
        strstr(pathname, "/fd/") != nullptr) {
        char linktarget[PATH_MAX];
        ssize_t len = ::readlink(pathname, linktarget, sizeof(linktarget) - 1);
        if (len > 0) {
            linktarget[len] = '\0';
            if (pathIsHidden(linktarget)) {
                ALOGD("access fd hidden: %s -> %s", pathname, linktarget);
                errno = ENOENT;
                return -1;
            }
        }
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

// ---- dl_iterate_phdr hook ----
// Modern packers and detectors use dl_iterate_phdr to enumerate all loaded
// shared libraries via the linker's internal list. We intercept this to
// skip over hidden libraries.

static int (*orig_dl_iterate_phdr)(int (*callback)(struct dl_phdr_info *, size_t, void *), void *data) = nullptr;

struct FilteredIterateContext {
    int (*orig_callback)(struct dl_phdr_info *, size_t, void *);
    void *orig_data;
};

static int filter_callback(struct dl_phdr_info *info, size_t size, void *data) {
    auto *ctx = static_cast<FilteredIterateContext *>(data);
    if (info->dlpi_name != nullptr && info->dlpi_name[0] != '\0') {
        if (pathIsHidden(info->dlpi_name)) {
            // Skip hidden libraries
            ALOGD("dl_iterate_phdr filtered: %s", info->dlpi_name);
            return 0; // 0 = continue to next entry
        }
    }
    return ctx->orig_callback(info, size, ctx->orig_data);
}

static int new_dl_iterate_phdr(int (*callback)(struct dl_phdr_info *, size_t, void *), void *data) {
    if (!g_enabled.load(std::memory_order_acquire)) {
        if (orig_dl_iterate_phdr) {
            return orig_dl_iterate_phdr(callback, data);
        }
        extern int dl_iterate_phdr(int (*)(struct dl_phdr_info *, size_t, void *), void *);
        return ::dl_iterate_phdr(callback, data);
    }
    FilteredIterateContext ctx = { callback, data };
    if (orig_dl_iterate_phdr) {
        return orig_dl_iterate_phdr(filter_callback, &ctx);
    }
    extern int dl_iterate_phdr(int (*)(struct dl_phdr_info *, size_t, void *), void *);
    return ::dl_iterate_phdr(filter_callback, &ctx);
}

}  // namespace

// ---- Public API ----

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
        g_hidden_paths.reserve(sizeof(kDefaultHiddenPaths) / sizeof(kDefaultHiddenPaths[0]));
        for (const char *p : kDefaultHiddenPaths) {
            g_hidden_paths.emplace_back(p);
        }
    }

    const char *soName = ".*\\.so$";

    // ---- dlopen family ----
    xhook_register(soName, "dlopen",
                   reinterpret_cast<void *>(new_dlopen), nullptr);
    xhook_register(soName, "android_dlopen_ext",
                   reinterpret_cast<void *>(new_android_dlopen_ext), nullptr);
    xhook_register(soName, "do_dlopen",
                   reinterpret_cast<void *>(new_do_dlopen), nullptr);

    // ---- file open family ----
    xhook_register(soName, "open",
                   reinterpret_cast<void *>(new_open), nullptr);
    xhook_register(soName, "openat",
                   reinterpret_cast<void *>(new_openat), nullptr);
    xhook_register(soName, "fopen",
                   reinterpret_cast<void *>(new_fopen), nullptr);
    xhook_register(soName, "fopen64",
                   reinterpret_cast<void *>(new_fopen64), nullptr);

    // ---- file existence/metadata ----
    xhook_register(soName, "access",
                   reinterpret_cast<void *>(new_access), nullptr);
    xhook_register(soName, "stat",
                   reinterpret_cast<void *>(new_stat), nullptr);
    xhook_register(soName, "fstatat",
                   reinterpret_cast<void *>(new_fstatat), nullptr);
    xhook_register(soName, "lstat",
                   reinterpret_cast<void *>(new_lstat), nullptr);
    xhook_register(soName, "stat64",
                   reinterpret_cast<void *>(new_stat64), nullptr);
    xhook_register(soName, "lstat64",
                   reinterpret_cast<void *>(new_lstat64), nullptr);

    // ---- symlink resolution ----
    xhook_register(soName, "readlink",
                   reinterpret_cast<void *>(new_readlink), nullptr);
    xhook_register(soName, "readlinkat",
                   reinterpret_cast<void *>(new_readlinkat), nullptr);

    xhook_refresh(0);

    // Install linker namespace hook via Dobby
    installLinkerNamespaceHook();

    ALOGD("SoHider: initialised with %zu name rules and %zu path rules",
          g_hidden_names.size(), g_hidden_paths.size());
}

void SoHider::setEnabled(bool enabled) {
    g_enabled.store(enabled, std::memory_order_release);
    ALOGD("SoHider: %s", enabled ? "enabled" : "disabled");
}

void SoHider::addHiddenName(const char *basename) {
    if (basename == nullptr) return;
    std::lock_guard<std::mutex> lock(g_hidden_mutex);
    std::string n(basename);
    if (std::find(g_hidden_names.begin(), g_hidden_names.end(), n) == g_hidden_names.end()) {
        g_hidden_names.emplace_back(std::move(n));
    }
}

void SoHider::addHiddenPath(const char *pathSubstring) {
    if (pathSubstring == nullptr) return;
    std::lock_guard<std::mutex> lock(g_hidden_mutex);
    std::string p(pathSubstring);
    if (std::find(g_hidden_paths.begin(), g_hidden_paths.end(), p) == g_hidden_paths.end()) {
        g_hidden_paths.emplace_back(std::move(p));
    }
}

void SoHider::installLinkerNamespaceHook() {
    if (g_linker_hook_installed.exchange(true)) return;

    // Hook dl_iterate_phdr in libdl.so (or linker) to filter library enumeration
    void *dl_iterate_addr = DobbySymbolResolver(nullptr, "dl_iterate_phdr");
    if (dl_iterate_addr) {
        _make_rwx(dl_iterate_addr, _page_size);
        DobbyHook(dl_iterate_addr,
                  reinterpret_cast<void *>(new_dl_iterate_phdr),
                  reinterpret_cast<void **>(&orig_dl_iterate_phdr));
        ALOGD("SoHider: dl_iterate_phdr hook installed");
    }
}

// ---- Utilities ----

namespace {

bool lineIsHidden(const char *line) {
    const char *path = strchr(line, '/');
    if (path == nullptr) return false;
    if (pathIsHidden(path)) return true;
    return false;
}

char *readProcFileFiltered(const char *procPath, size_t *length) {
    if (length != nullptr) *length = 0;
    FILE *f = fopen(procPath, "r");
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
            if (newcap > kCap) break;
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

}  // namespace

char *SoHider::readMapsFiltered(size_t *length) {
    return readProcFileFiltered("/proc/self/maps", length);
}

char *SoHider::readSmapsFiltered(size_t *length) {
    return readProcFileFiltered("/proc/self/smaps", length);
}

char *SoHider::readFdDirFiltered(size_t *length) {
    // List /proc/self/fd, filtering out entries pointing to hidden libraries
    if (length != nullptr) *length = 0;
    DIR *dir = opendir("/proc/self/fd");
    if (dir == nullptr) return nullptr;

    size_t cap = 4096;
    size_t used = 0;
    char *buf = static_cast<char *>(malloc(cap));
    if (buf == nullptr) {
        closedir(dir);
        return nullptr;
    }
    buf[0] = '\0';

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') continue; // skip . and ..

        char fdpath[PATH_MAX];
        snprintf(fdpath, sizeof(fdpath), "/proc/self/fd/%s", entry->d_name);
        char target[PATH_MAX];
        ssize_t targetLen = ::readlink(fdpath, target, sizeof(target) - 1);

        if (targetLen > 0) {
            target[targetLen] = '\0';
            if (pathIsHidden(target)) {
                // Skip this fd entry
                ALOGD("SoHider: fd filtered: %s -> %s", fdpath, target);
                continue;
            }
        }

        // Include this entry in output
        char line[PATH_MAX + 64];
        int lineLen = snprintf(line, sizeof(line), "%s -> %s\n",
                               entry->d_name, targetLen > 0 ? target : "(unknown)");
        if (used + lineLen + 1 > cap) {
            size_t newcap = cap * 2;
            if (newcap > 256 * 1024) break;
            char *nb = static_cast<char *>(realloc(buf, newcap));
            if (nb == nullptr) break;
            buf = nb;
            cap = newcap;
        }
        memcpy(buf + used, line, lineLen);
        used += lineLen;
    }
    closedir(dir);
    buf[used] = '\0';
    if (length != nullptr) *length = used;
    return buf;
}