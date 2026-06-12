package top.niunaijun.blackbox.utils;

import android.util.Log;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

/**
 * Dump process logger - writes to both logcat and a log file
 * so that dump failures can be diagnosed.
 */
public class DumpLogger {
    private static final String TAG = "BlackDex-Dump";
    private static File sLogFile;
    private static final SimpleDateFormat SDF =
            new SimpleDateFormat("HH:mm:ss.SSS", Locale.US);

    public static void init(File logFile) {
        sLogFile = logFile;
        if (sLogFile != null) {
            File parent = sLogFile.getParentFile();
            if (parent != null && !parent.exists()) {
                parent.mkdirs();
            }
        }
    }

    public static File getLogFile() {
        return sLogFile;
    }

    public static void i(String msg) {
        Log.i(TAG, msg);
        writeToFile("I", msg);
    }

    public static void e(String msg) {
        Log.e(TAG, msg);
        writeToFile("E", msg);
    }

    public static void e(String msg, Throwable t) {
        Log.e(TAG, msg, t);
        writeToFile("E", msg + " | " + Log.getStackTraceString(t));
    }

    public static void w(String msg) {
        Log.w(TAG, msg);
        writeToFile("W", msg);
    }

    private static synchronized void writeToFile(String level, String msg) {
        if (sLogFile == null) return;
        try {
            FileWriter writer = new FileWriter(sLogFile, true);
            String time = SDF.format(new Date());
            writer.write("[" + time + "] [" + level + "] " + msg + "\n");
            writer.flush();
            writer.close();
        } catch (IOException ignored) {
        }
    }

    public static String readLog() {
        if (sLogFile == null || !sLogFile.exists()) return "No log file";
        try {
            return top.niunaijun.blackbox.utils.FileUtils.readToString(sLogFile.getAbsolutePath());
        } catch (Exception e) {
            return "Failed to read log: " + e.getMessage();
        }
    }

    public static void clear() {
        if (sLogFile != null && sLogFile.exists()) {
            sLogFile.delete();
        }
    }
}
