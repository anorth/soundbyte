#include <jni.h>
#include <string.h>

#include "scom.h"

// Why do I need this for Eclipse?
int encodeMessage(char *payload, int payloadLength, char *waveform, int waveformCapacity);

// Export definitions for JNI
extern "C" {
  jstring Java_com_example_scom_nativ_Jni_stringFromJNI(JNIEnv* env, jobject thiz);
  void Java_com_example_scom_nativ_Jni_encodeMessage(JNIEnv *env, jobject thiz, jbyteArray payload,
      jobject forWaveform);
  void Java_com_example_scom_nativ_Jni_decodeAudio(JNIEnv *env, jobject thiz, jobject buffer);
  jboolean Java_com_example_scom_nativ_Jni_messageAvailable(JNIEnv *env, jobject thiz);
  jbyteArray Java_com_example_scom_nativ_Jni_takeMessage(JNIEnv *env, jobject thiz);
}

// Implementation

jstring Java_com_example_scom_nativ_Jni_stringFromJNI(JNIEnv* env, jobject thiz) {
  return env->NewStringUTF(STRING);
}

void Java_com_example_scom_nativ_Jni_encodeMessage(JNIEnv *env, jobject thiz, jbyteArray payload,
    jobject forWaveform) {
  // Marshal parameters
  jbyte *payloadBytes = env->GetByteArrayElements(payload, 0);
  int payloadLength = env->GetArrayLength(payload);
  char *waveformBuffer = (char *)env->GetDirectBufferAddress(forWaveform);
  int waveformBufferCapacity = env->GetDirectBufferCapacity(forWaveform);

  // Call into scom
  int bytesWritten = encodeMessage((char *)payloadBytes, payloadLength, waveformBuffer,
    waveformBufferCapacity);

  env->ReleaseByteArrayElements(payload, payloadBytes, 0);

  // Set bytebuffer limit
  jclass cls = env->GetObjectClass(forWaveform);
  jmethodID limitMethod = env->GetMethodID(cls, "limit", "(I)Ljava/nio/Buffer;");
  env->CallObjectMethod(forWaveform, limitMethod, bytesWritten);
}

//  public native void decodeAudio(ByteBuffer audio);

void Java_com_example_scom_nativ_Jni_decodeAudio(JNIEnv *env, jobject thiz, jobject buffer) {
  return;
}

//  public native boolean messageAvailable();
jboolean Java_com_example_scom_nativ_Jni_messageAvailable(JNIEnv *env, jobject thiz) {
  return false;
}

//  public native byte[] takeMessage();
jbyteArray Java_com_example_scom_nativ_Jni_takeMessage(JNIEnv *env, jobject thiz) {
  env->ThrowNew(env->FindClass("java.lang.RuntimeException"), "No message available");
  return 0;
}





