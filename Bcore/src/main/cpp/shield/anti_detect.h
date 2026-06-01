//
// Created by refactor on 2026/06/01.
// Anti-detection module header.
//

#ifndef BLACKBOX_ANTI_DETECT_H
#define BLACKBOX_ANTI_DETECT_H

#include <jni.h>
#include <stdint.h>

// xhook trampoline 宏,项目内 DexDump.h 也有定义,这里复刻一份
// 以便 shield 子模块不依赖外部头文件。
#ifndef AD_HOOK_FUN
#define AD_HOOK_FUN(ret, func, ...)            \
    ret (*orig_##func)(__VA_ARGS__);            \
    ret new_##func(__VA_ARGS__)
#endif

namespace shield {

/**
 * AntiDetect: 反检测模块。
 *
 * 设计原则:
 *   1. 复用项目内已有的 xhook 进行 PLT/GOT Hook,不引入额外依赖。
 *   2. 安装为幂等操作,重复调用不会重复注册。
 *   3. 提供 install/randomizeProcessName 两条独立入口,允许上层按需开启。
 *
 * 注意:
 *   本模块的过滤关键字由 kSensitiveKeywords 集中维护;
 *   任何包含这些关键字的 /proc/self/maps 行将被替换成空行,
 *   /proc/self/status 中的 TracerPid 会被强制置 0。
 */
class AntiDetect {
public:
    /**
     * 安装所有反检测 Hook(maps / status / network / etc.)。
     * @return 0 表示成功,非 0 表示至少有一项失败(不影响其他项继续安装)。
     */
    static int install();

    /**
     * 卸载所有 Hook,恢复原函数指针。
     * 注意:xhook 的 unregister API 在某些 so 模式下不保证 100% 还原,
     * 实际使用中通常无需调用。
     */
    static void uninstall();

    /**
     * 随机化当前进程名(通过 prctl(PR_SET_NAME, ...))。
     * 推荐在 JNI_OnLoad 之后立即调用,避免任何 /proc 检测看到原进程名。
     * @return 0 成功,-1 失败。
     */
    static int randomizeProcessName();

    /**
     * 当前是否已安装。
     */
    static int isInstalled();

    /**
     * 由 Java/Kotlin 侧调用的入口(对应 VMCore.installAntiDetect)。
     */
    static void jniInstall(JNIEnv *env, jclass clazz);

    static void jniRandomizeProcessName(JNIEnv *env, jclass clazz);

private:
    AntiDetect() = default;
    ~AntiDetect() = default;

    static int installMapsHook();
    static int installStatusHook();
    static int installNetworkHook();

    static volatile int sInstalled;
};

}  // namespace shield

#endif  // BLACKBOX_ANTI_DETECT_H
