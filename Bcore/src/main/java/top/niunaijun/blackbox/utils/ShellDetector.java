package top.niunaijun.blackbox.utils;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.text.TextUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * 壳识别器。
 *
 * 通过 (1) 特征类名 (2) so 文件名 (3) assets 中的资源名 三种维度
 * 识别当前被 dump 的目标应用是哪种壳。
 *
 * 注意:这是一个 best-effort 的指纹识别,只能用作 dump 策略的参考,
 * 不可作为绝对判断。
 */
public final class ShellDetector {

    private ShellDetector() {}

    public enum ShellType {
        UNKNOWN,
        QIHU_360,    // 360 加固
        TENCENT,     // 腾讯御安全
        AIJIAMI,     // 爱加密
        BANG_CLE,    // 邦邦
        BAIDU,       // 百度
        NAKE,        // 裸 APK(未加固)
    }

    /**
     * 指纹定义:包含(类名关键字, so 关键字)。
     * 命中任一维度即视为对应壳。
     */
    private static final class Fingerprint {
        final ShellType type;
        final String[] classHints;
        final String[] soHints;
        final String[] assetHints;

        Fingerprint(ShellType type, String[] classHints, String[] soHints, String[] assetHints) {
            this.type = type;
            this.classHints = classHints;
            this.soHints = soHints;
            this.assetHints = assetHints;
        }
    }

    private static final List<Fingerprint> FINGERPRINTS = new ArrayList<>();
    static {
        FINGERPRINTS.add(new Fingerprint(
                ShellType.QIHU_360,
                new String[]{"com.qihoo.util.StubClass", "com.qihoo.util.QHPlugin", "com.qihoo360.mobilesafe"},
                new String[]{"libjiagu", "libprotectClass", "libQihooProtect"},
                new String[]{"qihoo", "jiagu"}));
        FINGERPRINTS.add(new Fingerprint(
                ShellType.TENCENT,
                new String[]{"com.tencent.StubShell.TwApp", "com.tencent.bugly.legu", "com.tencent.mm"},
                new String[]{"libshell", "libtencent", "libtup", "libexec"},
                new String[]{"tencent", "tup", "shell"}));
        FINGERPRINTS.add(new Fingerprint(
                ShellType.AIJIAMI,
                new String[]{"com.shell.SuperShell", "com.aijiami.encrypt"},
                new String[]{"libexec", "libexecmain", "libjiagu"},
                new String[]{"aijiami"}));
        FINGERPRINTS.add(new Fingerprint(
                ShellType.BANG_CLE,
                new String[]{"com.bangcle.safebox", "com.bangcle.shell"},
                new String[]{"libsecexe", "libbangcle"},
                new String[]{"bangcle"}));
        FINGERPRINTS.add(new Fingerprint(
                ShellType.BAIDU,
                new String[]{"com.baidu.sapi2", "com.baidu.securez"},
                new String[]{"libbaiduprotect"},
                new String[]{"baidu"}));
    }

    /**
     * 根据 ApplicationInfo 与 classpath 判定壳类型。
     *
     * @param app 目标应用的 ApplicationInfo
     * @param baseClassLoader 目标应用的 ClassLoader(用于反射探测)
     */
    public static ShellType detect(ApplicationInfo app, ClassLoader baseClassLoader) {
        if (app == null) return ShellType.UNKNOWN;

        // 1. 扫描 nativeLibraryDir / splitSourceDirs
        List<String> paths = collectAllPaths(app);
        for (Fingerprint fp : FINGERPRINTS) {
            for (String path : paths) {
                for (String hint : fp.soHints) {
                    if (!TextUtils.isEmpty(hint) && path.contains(hint)) {
                        return fp.type;
                    }
                }
            }
        }

        // 2. 反射探测特征类
        if (baseClassLoader != null) {
            for (Fingerprint fp : FINGERPRINTS) {
                for (String classHint : fp.classHints) {
                    if (TextUtils.isEmpty(classHint)) continue;
                    try {
                        Class.forName(classHint, false, baseClassLoader);
                        return fp.type;
                    } catch (Throwable ignored) {
                    }
                }
            }
        }

        return ShellType.UNKNOWN;
    }

    /**
     * 便利重载:仅基于路径/so 名识别,不做类反射(快速路径)。
     */
    public static ShellType detectByPathOnly(ApplicationInfo app) {
        if (app == null) return ShellType.UNKNOWN;
        List<String> paths = collectAllPaths(app);
        for (Fingerprint fp : FINGERPRINTS) {
            for (String path : paths) {
                for (String hint : fp.soHints) {
                    if (!TextUtils.isEmpty(hint) && path.contains(hint)) {
                        return fp.type;
                    }
                }
            }
        }
        return ShellType.UNKNOWN;
    }

    private static List<String> collectAllPaths(ApplicationInfo app) {
        List<String> paths = new ArrayList<>();
        if (!TextUtils.isEmpty(app.sourceDir)) paths.add(app.sourceDir);
        if (app.splitSourceDirs != null) {
            for (String s : app.splitSourceDirs) paths.add(s);
        }
        if (!TextUtils.isEmpty(app.nativeLibraryDir)) paths.add(app.nativeLibraryDir);
        // 追加 nativeLibraryDir 下的 so 列表
        if (!TextUtils.isEmpty(app.nativeLibraryDir)) {
            File dir = new File(app.nativeLibraryDir);
            File[] children = dir.listFiles();
            if (children != null) {
                for (File f : children) paths.add(f.getAbsolutePath());
            }
        }
        return paths;
    }

    /**
     * 是否已加固(非 NAKE 且非 UNKNOWN)。
     */
    public static boolean isPacked(ShellType type) {
        return type != ShellType.UNKNOWN && type != ShellType.NAKE;
    }
}
