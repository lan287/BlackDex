package top.niunaijun.blackbox;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.net.Uri;
import android.os.Process;

import java.io.File;
import java.util.List;

import top.niunaijun.blackbox.app.configuration.ClientConfiguration;
import top.niunaijun.blackbox.core.system.dump.IBDumpMonitor;
import top.niunaijun.blackbox.entity.pm.InstallResult;
import top.niunaijun.blackbox.proxy.ProxyManifest;
import top.niunaijun.blackbox.utils.DumpLogger;

/**
 * Created by Milk on 2021/5/22.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class BlackDexCore {
    public static final String TAG = "BlackBoxCore";

    private static final BlackDexCore sBlackDexCore = new BlackDexCore();

    public static BlackDexCore get() {
        return sBlackDexCore;
    }

    public void doAttachBaseContext(Context context, ClientConfiguration clientConfiguration) {
        BlackBoxCore.get().doAttachBaseContext(context, clientConfiguration);
    }

    public void doCreate() {
        BlackBoxCore.get().doCreate();
        // uninstall all pckage
        if (BlackBoxCore.get().isMainProcess()) {
            List<PackageInfo> installedPackages =
                    BlackBoxCore.getBPackageManager().getInstalledPackages(0, BlackBoxCore.USER_ID);
            for (PackageInfo installedPackage : installedPackages) {
                BlackBoxCore.get().uninstallPackage(installedPackage.packageName);
            }
        }
    }

    public InstallResult dumpDex(String packageName) {
        DumpLogger.init(new File(BlackBoxCore.get().getDexDumpDir(), "dump.log"));
        DumpLogger.i("=== dumpDex(String) START === packageName=" + packageName);
        InstallResult installResult = BlackBoxCore.get().installPackage(packageName);
        DumpLogger.i("installPackage result: success=" + (installResult != null && installResult.success) + " packageName=" + (installResult != null ? installResult.packageName : "null"));
        if (installResult.success) {
            boolean b = BlackBoxCore.get().launchApk(packageName);
            DumpLogger.i("launchApk result: " + b);
            if (!b) {
                DumpLogger.e("launchApk failed, uninstalling");
                BlackBoxCore.get().uninstallPackage(installResult.packageName);
                return null;
            }
            return installResult;
        } else {
            DumpLogger.e("installPackage failed");
            return null;
        }
    }

    public InstallResult dumpDex(File file) {
        DumpLogger.init(new File(BlackBoxCore.get().getDexDumpDir(), "dump.log"));
        DumpLogger.i("=== dumpDex(File) START === file=" + file.getAbsolutePath());
        InstallResult installResult = BlackBoxCore.get().installPackage(file);
        DumpLogger.i("installPackage result: success=" + (installResult != null && installResult.success));
        if (installResult.success) {
            boolean b = BlackBoxCore.get().launchApk(installResult.packageName);
            DumpLogger.i("launchApk result: " + b);
            if (!b) {
                DumpLogger.e("launchApk failed, uninstalling");
                BlackBoxCore.get().uninstallPackage(installResult.packageName);
                return null;
            }
            return installResult;
        } else {
            DumpLogger.e("installPackage failed");
            return null;
        }
    }

    public InstallResult dumpDex(Uri file) {
        DumpLogger.init(new File(BlackBoxCore.get().getDexDumpDir(), "dump.log"));
        DumpLogger.i("=== dumpDex(Uri) START === uri=" + file.toString());
        InstallResult installResult = BlackBoxCore.get().installPackage(file);
        DumpLogger.i("installPackage result: success=" + (installResult != null && installResult.success));
        if (installResult.success) {
            boolean b = BlackBoxCore.get().launchApk(installResult.packageName);
            DumpLogger.i("launchApk result: " + b);
            if (!b) {
                DumpLogger.e("launchApk failed, uninstalling");
                BlackBoxCore.get().uninstallPackage(installResult.packageName);
                return null;
            }
            return installResult;
        } else {
            DumpLogger.e("installPackage failed");
            return null;
        }
    }

    public void registerDumpMonitor(IBDumpMonitor monitor) {
        BlackBoxCore.getBDumpManager().registerMonitor(monitor.asBinder());
    }

    public void unregisterDumpMonitor(IBDumpMonitor monitor) {
        BlackBoxCore.getBDumpManager().unregisterMonitor(monitor.asBinder());
    }

    public boolean isRunning() {
        ActivityManager am = (ActivityManager) BlackBoxCore.getContext().getSystemService(Context.ACTIVITY_SERVICE);
        for (ActivityManager.RunningAppProcessInfo info : am.getRunningAppProcesses()) {
            for (int i = 0; i < ProxyManifest.FREE_COUNT; i++) {
                if (info.processName.endsWith("p" + i)) {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isExistDexFile(String packageName) {
        File dir = new File(BlackBoxCore.get().getDexDumpDir(), packageName);
        File[] files = dir.listFiles();
        if (files == null) return false;
        for (File f : files) {
            if (f.isFile() && f.getName().endsWith(".dex") && f.length() > 0x70) {
                return true;
            }
        }
        return false;
    }
}
