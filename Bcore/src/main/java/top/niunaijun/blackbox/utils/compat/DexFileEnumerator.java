package top.niunaijun.blackbox.utils.compat;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import dalvik.system.BaseDexClassLoader;
import dalvik.system.DexFile;
import top.niunaijun.blackbox.BlackBoxCore;
import top.niunaijun.blackbox.utils.Reflector;

/**
 * 全量 ClassLoader / DexFile 枚举器。
 *
 * 与 {@link DexFileCompat} 的区别:
 *   - DexFileCompat:只针对单个 ClassLoader 进行 dexElements 反射
 *   - DexFileEnumerator:遍历进程内所有已知 ClassLoader 树,合并去重
 *
 * 覆盖范围:
 *   1. ActivityThread.mPackages 中所有 LoadedApk.mClassLoader
 *   2. 所有运行线程的 contextClassLoader
 *   3. 通过 classLoader.getParent() 递归
 *
 * 注意:本类只做枚举,不做 dump。dump 逻辑仍由 VMCore.cookieDumpDex 承担。
 */
public final class DexFileEnumerator {

    private DexFileEnumerator() {}

    public static final class Result {
        public final List<DexFile> dexFiles;
        public final List<ClassLoader> classLoaders;

        public Result(List<DexFile> dexFiles, List<ClassLoader> classLoaders) {
            this.dexFiles = dexFiles;
            this.classLoaders = classLoaders;
        }
    }

    /**
     * 枚举当前进程内可见的所有 DexFile。
     */
    public static Result enumerateAll() {
        List<DexFile> dexFiles = new ArrayList<>();
        Set<ClassLoader> visited = new HashSet<>();
        List<ClassLoader> classLoaders = new ArrayList<>();

        collectFromActivityThread(visited, classLoaders, dexFiles);
        collectFromThreads(visited, classLoaders, dexFiles);

        return new Result(dexFiles, classLoaders);
    }

    /**
     * 从 ActivityThread.mPackages -> LoadedApk.mClassLoader 入手。
     */
    private static void collectFromActivityThread(Set<ClassLoader> visited,
                                                  List<ClassLoader> classLoaders,
                                                  List<DexFile> dexFiles) {
        try {
            Object activityThread = BlackBoxCore.mainThread();
            if (activityThread == null) return;

            Object packages = Reflector.with(activityThread)
                    .field("mPackages")
                    .get();
            if (!(packages instanceof Map)) return;

            for (Object loadedApk : ((Map<?, ?>) packages).values()) {
                if (loadedApk == null) continue;
                try {
                    Object cl = Reflector.with(loadedApk)
                            .field("mClassLoader")
                            .get();
                    if (cl instanceof ClassLoader) {
                        walkClassLoader((ClassLoader) cl, visited, classLoaders, dexFiles);
                    }
                } catch (Throwable ignored) {
                }
            }
        } catch (Throwable t) {
            t.printStackTrace();
        }
    }

    /**
     * 遍历所有线程的 contextClassLoader。
     */
    private static void collectFromThreads(Set<ClassLoader> visited,
                                           List<ClassLoader> classLoaders,
                                           List<DexFile> dexFiles) {
        try {
            Map<Thread, StackTraceElement[]> all = Thread.getAllStackTraces();
            for (Thread t : all.keySet()) {
                try {
                    ClassLoader cl = t.getContextClassLoader();
                    if (cl != null) {
                        walkClassLoader(cl, visited, classLoaders, dexFiles);
                    }
                } catch (Throwable ignored) {
                }
            }
        } catch (Throwable t) {
            t.printStackTrace();
        }
    }

    /**
     * 递归遍历 ClassLoader 树,从中提取 DexFile。
     * 处理 BaseDexClassLoader(API 1+)、InMemoryDexClassLoader(API 26+)、
     * DelegateLastClassLoader(API 28+) 等。
     */
    private static void walkClassLoader(ClassLoader cl,
                                        Set<ClassLoader> visited,
                                        List<ClassLoader> classLoaders,
                                        List<DexFile> dexFiles) {
        if (cl == null) return;
        if (!visited.add(cl)) return;
        classLoaders.add(cl);

        if (cl instanceof BaseDexClassLoader) {
            try {
                Object pathList = Reflector.on("dalvik.system.BaseDexClassLoader")
                        .field("pathList")
                        .get(cl);
                if (pathList != null) {
                    Object[] elements = Reflector.with(pathList)
                            .field("dexElements")
                            .get();
                    if (elements != null) {
                        for (Object element : elements) {
                            try {
                                DexFile df = Reflector.with(element)
                                        .field("dexFile")
                                        .<DexFile>get();
                                if (df != null && !dexFiles.contains(df)) {
                                    dexFiles.add(df);
                                }
                            } catch (Throwable ignored) {
                            }
                        }
                    }
                }
            } catch (Throwable t) {
                t.printStackTrace();
            }
        } else {
            // 兜底:某些 ClassLoader(如 InMemoryDexClassLoader)不继承 BaseDexClassLoader
            // 但仍通过 getDeclaredField("dexFile") 暴露 DexFile。
            try {
                Object df = Reflector.with(cl)
                        .field("dexFile")
                        .get();
                if (df instanceof DexFile && !dexFiles.contains(df)) {
                    dexFiles.add((DexFile) df);
                }
            } catch (Throwable ignored) {
            }
        }

        try {
            walkClassLoader(cl.getParent(), visited, classLoaders, dexFiles);
        } catch (Throwable ignored) {
        }
    }

    /**
     * 仅从单一 ClassLoader 提取 DexFile 的便捷方法,内部委托给 DexFileCompat。
     */
    public static List<DexFile> getDexFilesFrom(ClassLoader cl) {
        return new ArrayList<>(DexFileCompat.getDexFiles(cl));
    }
}
