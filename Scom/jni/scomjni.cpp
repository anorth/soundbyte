#include <jni.h>
#include <string.h>

#include "scom.h"

extern "C" {
  jstring Java_com_example_scom_nativ_Jni_stringFromJNI(JNIEnv* env, jobject thiz);
}

jstring
Java_com_example_scom_nativ_Jni_stringFromJNI(JNIEnv* env, jobject thiz) {
    return env->NewStringUTF(STRING);
}
