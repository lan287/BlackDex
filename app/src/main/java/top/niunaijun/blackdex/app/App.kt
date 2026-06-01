package top.niunaijun.blackdex.app

import android.annotation.SuppressLint
import android.app.Application
import android.content.Context
import com.umeng.commonsdk.UMConfigure
import top.niunaijun.blackbox.core.VMCore

/**
 *
 * @Description:
 * @Author: wukaicheng
 * @CreateDate: 2021/4/29 21:21
 */
class App : Application() {

    companion object {

        @SuppressLint("StaticFieldLeak")
        @Volatile
        private lateinit var mContext: Context

        @JvmStatic
        fun getContext(): Context {
            return mContext
        }
    }

    override fun attachBaseContext(base: Context?) {
        super.attachBaseContext(base)
        mContext = base!!
        AppManager.doAttachBaseContext(base)
        // 反检测:在最早阶段随机化进程名,降低 /proc 检测命中率。
        // 调用失败时静默吞掉,不影响主流程。
        runCatching { VMCore.randomizeProcName() }
    }

    override fun onCreate() {
        super.onCreate()
        AppManager.doOnCreate(mContext)
        UMConfigure.init(this, "60b373136c421a3d97d23c29", "Github", UMConfigure.DEVICE_TYPE_PHONE, null)
        // 反检测 Hook 在主流程初始化完成后再装,避免影响反射调用。
        runCatching { VMCore.installAntiDetect() }
    }
}