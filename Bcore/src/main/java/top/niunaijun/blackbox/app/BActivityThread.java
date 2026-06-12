package top.niunaijun.blackbox.app;

import android.app.Application;
import android.app.Instrumentation;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ProviderInfo;
import android.os.Build;
import android.os.ConditionVariable;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.List;
import java.util.Objects;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import reflection.android.app.ActivityThread;
import reflection.android.app.ContextImpl;
import reflection.android.app.LoadedApk;
import top.niunaijun.blackbox.core.IBActivityThread;
import top.niunaijun.blackbox.core.VMCore;
import top.niunaijun.blackbox.entity.AppConfig;
import top.niunaijun.blackbox.core.IOCore;
import top.niunaijun.blackbox.entity.dump.DumpResult;
import top.niunaijun.blackbox.utils.FileUtils;
import top.niunaijun.blackbox.utils.Slog;
import top.niunaijun.blackbox.utils.DumpLogger;
import top.niunaijun.blackbox.BlackBoxCore;

/**
 * Created by Milk on 3/31/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class BActivityThread extends IBActivityThread.Stub {
    public static final String TAG = "BActivityThread";

    private static BActivityThread sBActivityThread;
    private AppBindData mBoundApplication;
    private Application mInitialApplication;
    private AppConfig mAppConfig;
    private final List<ProviderInfo> mProviders = new ArrayList<>();

    public static BActivityThread currentActivityThread() {
        if (sBActivityThread == null) {
            synchronized (BActivityThread.class) {
                if (sBActivityThread == null) {
                    sBActivityThread = new BActivityThread();
                }
            }
        }
        return sBActivityThread;
    }

    public static synchronized AppConfig getAppConfig() {
        return currentActivityThread().mAppConfig;
    }

    public static List<ProviderInfo> getProviders() {
        return currentActivityThread().mProviders;
    }

    public static String getAppProcessName() {
        if (getAppConfig() != null) {
            return getAppConfig().processName;
        } else if (currentActivityThread().mBoundApplication != null) {
            return currentActivityThread().mBoundApplication.processName;
        } else {
            return null;
        }
    }

    public static String getAppPackageName() {
        if (getAppConfig() != null) {
            return getAppConfig().packageName;
        } else if (currentActivityThread().mInitialApplication != null) {
            return currentActivityThread().mInitialApplication.getPackageName();
        } else {
            return null;
        }
    }

    public static Application getApplication() {
        return currentActivityThread().mInitialApplication;
    }

    public static int getAppPid() {
        return getAppConfig() == null ? -1 : getAppConfig().bpid;
    }

    public static int getAppUid() {
        return getAppConfig() == null ? 10000 : getAppConfig().buid;
    }

    public static int getBaseAppUid() {
        return getAppConfig() == null ? 10000 : getAppConfig().baseBUid;
    }

    public static int getUid() {
        return getAppConfig() == null ? -1 : getAppConfig().uid;
    }

    public static int getUserId() {
        return getAppConfig() == null ? 0 : getAppConfig().userId;
    }

    public void initProcess(AppConfig appConfig) {
        if (this.mAppConfig != null) {
            throw new RuntimeException("reject init process: " + appConfig.processName + ", this process is : " + this.mAppConfig.processName);
        }
        this.mAppConfig = appConfig;
    }

    public boolean isInit() {
        return mBoundApplication != null;
    }

    public void bindApplication(final String packageName, final String processName) {
        if (mAppConfig == null) {
            return;
        }
        if (Looper.myLooper() != Looper.getMainLooper()) {
            final ConditionVariable conditionVariable = new ConditionVariable();
            new Handler(Looper.getMainLooper()).post(() -> {
                handleBindApplication(packageName, processName);
                conditionVariable.open();
            });
            conditionVariable.block();
        } else {
            handleBindApplication(packageName, processName);
        }
    }

    private synchronized void handleBindApplication(String packageName, String processName) {
        DumpLogger.init(new File(BlackBoxCore.get().getDexDumpDir(), "dump.log"));
        DumpLogger.clear();
        DumpLogger.i("=== handleBindApplication START ===");
        DumpLogger.i("packageName=" + packageName + ", processName=" + processName);

        DumpResult result = new DumpResult();
        result.packageName = packageName;
        result.dir = new File(BlackBoxCore.get().getDexDumpDir(), packageName).getAbsolutePath();
        DumpLogger.i("dumpDir=" + result.dir);
        try {
            PackageInfo packageInfo = BlackBoxCore.getBPackageManager().getPackageInfo(packageName, PackageManager.GET_PROVIDERS, BActivityThread.getUserId());
            if (packageInfo == null) {
                DumpLogger.e("getPackageInfo returned null for " + packageName);
                return;
            }
            DumpLogger.i("getPackageInfo OK, sourceDir=" + (packageInfo.applicationInfo != null ? packageInfo.applicationInfo.sourceDir : "null"));
            ApplicationInfo applicationInfo = packageInfo.applicationInfo;
            if (packageInfo.providers == null) {
                packageInfo.providers = new ProviderInfo[]{};
            }
            mProviders.addAll(Arrays.asList(packageInfo.providers));

            Object boundApplication = ActivityThread.mBoundApplication.get(BlackBoxCore.mainThread());

            Context packageContext = createPackageContext(applicationInfo);
            DumpLogger.i("createPackageContext: " + (packageContext != null ? "OK" : "FAILED"));
            Object loadedApk = ContextImpl.mPackageInfo.get(packageContext);
            LoadedApk.mSecurityViolation.set(loadedApk, false);
            // fix applicationInfo
            LoadedApk.mApplicationInfo.set(loadedApk, applicationInfo);

            // clear dump file
            FileUtils.deleteDir(new File(BlackBoxCore.get().getDexDumpDir(), packageName));
            DumpLogger.i("Cleared old dump dir");

            // init vmCore
            DumpLogger.i("Initializing VMCore (api=" + Build.VERSION.SDK_INT + ")");
            VMCore.init(Build.VERSION.SDK_INT);
            DumpLogger.i("VMCore.init done");
            assert packageContext != null;
            IOCore.get().enableRedirect(packageContext);
            DumpLogger.i("IOCore redirect enabled");

            AppBindData bindData = new AppBindData();
            bindData.appInfo = applicationInfo;
            bindData.processName = processName;
            bindData.info = loadedApk;
            bindData.providers = mProviders;

            ActivityThread.AppBindData.instrumentationName.set(boundApplication,
                    new ComponentName(bindData.appInfo.packageName, Instrumentation.class.getName()));
            ActivityThread.AppBindData.appInfo.set(boundApplication, bindData.appInfo);
            ActivityThread.AppBindData.info.set(boundApplication, bindData.info);
            ActivityThread.AppBindData.processName.set(boundApplication, bindData.processName);
            ActivityThread.AppBindData.providers.set(boundApplication, bindData.providers);

            mBoundApplication = bindData;

            Application application = null;
            BlackBoxCore.get().getAppLifecycleCallback().beforeCreateApplication(packageName, processName, packageContext);
            try {
                ClassLoader call = LoadedApk.getClassloader.call(loadedApk);
                DumpLogger.i("Got classloader: " + (call != null ? call.getClass().getName() : "null"));
                application = LoadedApk.makeApplication.call(loadedApk, false, null);
                DumpLogger.i("makeApplication: " + (application != null ? application.getClass().getName() : "null"));
            } catch (Throwable e) {
                DumpLogger.e("Unable to makeApplication", e);
                Slog.e(TAG, "Unable to makeApplication");
                e.printStackTrace();
            }
            mInitialApplication = application;
            ActivityThread.mInitialApplication.set(BlackBoxCore.mainThread(), mInitialApplication);
            DumpLogger.i("isMainProcess=" + Objects.equals(packageName, processName));
            if (Objects.equals(packageName, processName)) {
                ClassLoader loader;
                if (application == null) {
                    loader = LoadedApk.getClassloader.call(loadedApk);
                } else {
                    loader = application.getClassLoader();
                }
                DumpLogger.i("Starting handleDumpDex with classloader=" + (loader != null ? loader.getClass().getName() : "null"));
                handleDumpDex(packageName, result, loader);
            } else {
                DumpLogger.i("Not main process, skip dump");
            }
        } catch (Throwable e) {
            DumpLogger.e("handleBindApplication FAILED", e);
            e.printStackTrace();
            mAppConfig = null;
            BlackBoxCore.getBDumpManager().noticeMonitor(result.dumpError(e.getMessage()));
            BlackBoxCore.get().uninstallPackage(packageName);
        }
    }

    private void handleDumpDex(String packageName, DumpResult result, ClassLoader classLoader) {
        DumpLogger.i("=== handleDumpDex START ===");
        DumpLogger.i("classLoader=" + (classLoader != null ? classLoader.getClass().getName() : "null"));
        new Thread(() -> {
            // Wait for hooks to capture dynamically loaded dex files
            // Packed apps load real dex in Application.onCreate() or Activity lifecycle
            boolean isFixCodeItem = BlackBoxCore.get().isFixCodeItem();
            long waitMs = isFixCodeItem ? 10000 : 3000;
            DumpLogger.i("Waiting " + waitMs + "ms for hooks (isFixCodeItem=" + isFixCodeItem + ")");
            try {
                Thread.sleep(waitMs);
            } catch (InterruptedException ignored) {
            }
            DumpLogger.i("Wait done, starting cookieDumpDex");

            try {
                VMCore.cookieDumpDex(classLoader, packageName);
                DumpLogger.i("cookieDumpDex completed");
            } catch (Throwable e) {
                DumpLogger.e("cookieDumpDex failed", e);
                Slog.e(TAG, "cookieDumpDex failed", e);
            }

            File dir = new File(result.dir);
            DumpLogger.i("After cookieDumpDex, checking dir: " + dir.getAbsolutePath() + " exists=" + dir.exists());

            // If native dump produced nothing, try extracting dex directly from APK
            if (!hasDexFiles(dir)) {
                DumpLogger.i("Native dump empty, trying APK extraction for " + packageName);
                try {
                    extractDexFromApk(packageName, dir);
                } catch (Throwable e) {
                    DumpLogger.e("APK extraction failed", e);
                    Slog.e(TAG, "APK extraction failed", e);
                }
            } else {
                DumpLogger.i("Native dump has dex files, skip APK extraction");
            }

            // List all files in dump dir
            if (dir.exists()) {
                File[] files = dir.listFiles();
                if (files != null) {
                    DumpLogger.i("Dump dir contains " + files.length + " files:");
                    for (File f : files) {
                        DumpLogger.i("  " + f.getName() + " (" + f.length() + " bytes)");
                    }
                } else {
                    DumpLogger.i("Dump dir listFiles returned null");
                }
            }

            mAppConfig = null;
            if (hasDexFiles(dir)) {
                DumpLogger.i("=== DUMP SUCCESS ===");
                BlackBoxCore.getBDumpManager().noticeMonitor(result.dumpSuccess());
            } else {
                DumpLogger.e("=== DUMP FAILED: no valid dex files found ===");
                BlackBoxCore.getBDumpManager().noticeMonitor(result.dumpError("not found dex file"));
            }
            BlackBoxCore.get().uninstallPackage(packageName);
        }).start();
    }

    private static boolean hasDexFiles(File dir) {
        if (!dir.exists()) return false;
        File[] files = dir.listFiles();
        if (files == null) return false;
        for (File f : files) {
            if (f.isFile() && f.getName().endsWith(".dex") && f.length() > 0x70) {
                return true;
            }
        }
        return false;
    }

    /**
     * Fallback: extract dex files directly from the APK.
     * This works for non-packed apps. For packed apps, the native hooks
     * should have already captured the dynamically loaded dex files.
     */
    private void extractDexFromApk(String packageName, File dumpDir) {
        try {
            PackageInfo packageInfo = BlackBoxCore.getBPackageManager().getPackageInfo(
                    packageName, 0, BActivityThread.getUserId());
            if (packageInfo == null || packageInfo.applicationInfo == null) {
                DumpLogger.e("extractDexFromApk: packageInfo or applicationInfo is null");
                return;
            }

            String apkPath = packageInfo.applicationInfo.sourceDir;
            if (apkPath == null) {
                DumpLogger.e("extractDexFromApk: sourceDir is null");
                return;
            }
            DumpLogger.i("extractDexFromApk: apkPath=" + apkPath);
            File apkFile = new File(apkPath);
            DumpLogger.i("extractDexFromApk: apkFile exists=" + apkFile.exists() + " size=" + apkFile.length());

            FileUtils.mkdirs(dumpDir.getAbsolutePath());
            ZipFile zipFile = new ZipFile(apkPath);
            Enumeration<? extends ZipEntry> entries = zipFile.entries();
            int dexCount = 0;
            while (entries.hasMoreElements()) {
                ZipEntry entry = entries.nextElement();
                String name = entry.getName();
                if (name.startsWith("classes") && name.endsWith(".dex")) {
                    File outputFile = new File(dumpDir, "apk_" + name.replace("/", "_"));
                    if (outputFile.exists()) continue;
                    try (InputStream is = zipFile.getInputStream(entry);
                         FileOutputStream fos = new FileOutputStream(outputFile)) {
                        byte[] buffer = new byte[8192];
                        int len;
                        while ((len = is.read(buffer)) > 0) {
                            fos.write(buffer, 0, len);
                        }
                    }
                    dexCount++;
                    DumpLogger.i("Extracted " + name + " from APK (" + outputFile.length() + " bytes)");
                }
            }
            zipFile.close();
            DumpLogger.i("extractDexFromApk: extracted " + dexCount + " dex files");
        } catch (Throwable e) {
            DumpLogger.e("extractDexFromApk error", e);
            Slog.e(TAG, "extractDexFromApk error", e);
        }
    }

    private Context createPackageContext(ApplicationInfo info) {
        try {
            return BlackBoxCore.getContext().createPackageContext(info.packageName,
                    Context.CONTEXT_INCLUDE_CODE | Context.CONTEXT_IGNORE_SECURITY);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    @Override
    public IBinder getActivityThread() {
        return ActivityThread.getApplicationThread.call(BlackBoxCore.mainThread());
    }

    @Override
    public void bindApplication() {
        if (!isInit()) {
            bindApplication(getAppPackageName(), getAppProcessName());
        }
    }

    public static class AppBindData {
        String processName;
        ApplicationInfo appInfo;
        List<ProviderInfo> providers;
        Object info;
    }
}
