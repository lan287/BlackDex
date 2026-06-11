//
// Created by Milk on 4/9/21.
// Enhanced for modern ART versions and deep dump support.
//

#ifndef VIRTUALM_VMCORE_H
#define VIRTUALM_VMCORE_H

#include <jni.h>

#define VMCORE_CLASS "top/niunaijun/blackbox/core/VMCore"

class VmCore {
public:
    static JavaVM *getJavaVM();
    static int getApiLevel();
    static jobject findMethod(JNIEnv *env, const char *className, const char *methodName, const char *signature);
    static int getCallingUid(JNIEnv *env, int orig);
    static jstring redirectPathString(JNIEnv *env, jstring path);
    static jobject redirectPathFile(JNIEnv *env, jobject path);
    static jlongArray loadEmptyDex(JNIEnv *env);
    static bool is64Bit();
};

#endif //VIRTUALM_VMCORE_H