#include <jni.h>
#include <string.h>

extern "C" {
  jstring Java_com_example_scom_Jni_stringFromJNI(JNIEnv* env, jobject thiz);
}


jstring
Java_com_example_scom_Jni_stringFromJNI(JNIEnv* env, jobject thiz) {
    return env->NewStringUTF("Hello from JNI!");
}
