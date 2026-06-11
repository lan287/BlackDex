package top.niunaijun.blackdex.data

import android.content.pm.ApplicationInfo
import android.net.Uri
import android.webkit.URLUtil
import androidx.lifecycle.MutableLiveData
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import top.niunaijun.blackbox.BlackBoxCore
import top.niunaijun.blackbox.BlackBoxCore.getPackageManager
import top.niunaijun.blackbox.BlackDexCore
import top.niunaijun.blackbox.entity.pm.InstallResult
import top.niunaijun.blackbox.utils.AbiUtils
import top.niunaijun.blackdex.R
import top.niunaijun.blackdex.app.App
import top.niunaijun.blackdex.app.AppManager
import top.niunaijun.blackdex.data.entity.AppInfo
import top.niunaijun.blackdex.data.entity.DumpInfo
import java.io.File

/**
 *
 * @Description:
 * @Author: wukaicheng
 * @CreateDate: 2021/5/23 14:29
 */
class DexDumpRepository {

    private var dumpTaskId = 0

    fun getAppList(mAppListLiveData: MutableLiveData<List<AppInfo>>) {
        GlobalScope.launch(Dispatchers.IO) {
            val installedApplications: List<ApplicationInfo> =
                    getPackageManager().getInstalledApplications(0)
            val installedList = mutableListOf<AppInfo>()

            for (installedApplication in installedApplications) {
                val file = File(installedApplication.sourceDir)

                if ((installedApplication.flags and ApplicationInfo.FLAG_SYSTEM) != 0) continue

                if (!AbiUtils.isSupport(file)) continue

                val label = installedApplication.loadLabel(getPackageManager()).toString()
                val icon = installedApplication.loadIcon(getPackageManager())
                val info = AppInfo(label, installedApplication.packageName, icon)
                installedList.add(info)
            }

            withContext(Dispatchers.Main) {
                mAppListLiveData.value = installedList
            }
        }
    }

    fun dumpDex(source: String, dexDumpLiveData: MutableLiveData<DumpInfo>) {
        dexDumpLiveData.postValue(DumpInfo(DumpInfo.LOADING))
        val result = if (URLUtil.isValidUrl(source)) {
            BlackDexCore.get().dumpDex(Uri.parse(source))
        } else if (source.contains("/")) {
            BlackDexCore.get().dumpDex(File(source))
        } else {
            BlackDexCore.get().dumpDex(source)
        }

        if (result != null) {
            dumpTaskId++
            startCountdown(result, dexDumpLiveData)
        } else {
            dexDumpLiveData.postValue(DumpInfo(DumpInfo.TIMEOUT))
        }
    }


    fun dumpSuccess() {
        dumpTaskId++
    }

    private fun startCountdown(installResult: InstallResult, dexDumpLiveData: MutableLiveData<DumpInfo>) {
        GlobalScope.launch {
            val tempId = dumpTaskId
            // Wait for the proxy process to actually start
            var waitCount = 0
            while (!BlackDexCore.get().isRunning && waitCount < 30) {
                delay(500)
                waitCount++
            }
            // Now wait for the proxy process to finish (or timeout)
            val isFixCode = AppManager.mBlackBoxLoader.isFixCodeItem()
            var timeoutCount = 0
            val maxTimeout = if (isFixCode) 120 else 60 // 60s for normal, 120s for fixCodeItem
            while (BlackDexCore.get().isRunning && timeoutCount < maxTimeout) {
                delay(1000)
                timeoutCount++
                // Check if we already have dex files (early exit)
                if (BlackDexCore.get().isExistDexFile(installResult.packageName)) {
                    break
                }
            }
            if (tempId == dumpTaskId) {
                if (BlackDexCore.get().isExistDexFile(installResult.packageName)) {
                    dexDumpLiveData.postValue(DumpInfo(
                            DumpInfo.SUCCESS,
                            App.getContext().getString(R.string.dex_save, File(BlackBoxCore.get().dexDumpDir, installResult.packageName).absolutePath)
                    ))
                } else {
                    dexDumpLiveData.postValue(DumpInfo(DumpInfo.TIMEOUT))
                }
            }
        }
    }
}