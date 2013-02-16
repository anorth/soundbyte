#include <jni.h>
#include <string.h>
#include <assert.h>

#include "scom.h"
#include "log.h"

// Export definitions for JNI
extern "C" {
  jstring Java_com_example_scom_nativ_Jni_stringFromJNI(JNIEnv* env, jobject thiz);
  void Java_com_example_scom_nativ_Jni_init(JNIEnv* env, jobject thiz,
      jint base, jint chipRate, jint channelSpacing, jint numChannels);
  void Java_com_example_scom_nativ_Jni_encodeMessage(JNIEnv *env, jobject thiz, jbyteArray payload,
      jobject forWaveform);
  void Java_com_example_scom_nativ_Jni_decodeAudio(JNIEnv *env, jobject thiz, jobject buffer);
  jboolean Java_com_example_scom_nativ_Jni_messageAvailable(JNIEnv *env, jobject thiz);
  jint Java_com_example_scom_nativ_Jni_takeMessage(JNIEnv *env, jobject thiz, jbyteArray target);
}

// Implementation

jstring Java_com_example_scom_nativ_Jni_stringFromJNI(JNIEnv* env, jobject thiz) {
  return env->NewStringUTF(HELLO);
}

void Java_com_example_scom_nativ_Jni_init(JNIEnv* env, jobject thiz,
    jint base, jint chipRate, jint channelSpacing, jint numChannels) {
  scomInit(base, chipRate, channelSpacing, numChannels);
}

void Java_com_example_scom_nativ_Jni_encodeMessage(JNIEnv *env, jobject thiz, jbyteArray payload,
    jobject forWaveform) {
  // Marshal parameters
  jbyte *payloadBytes = env->GetByteArrayElements(payload, 0);
  int payloadLength = env->GetArrayLength(payload);
  char *waveformBuffer = (char *)env->GetDirectBufferAddress(forWaveform);
  int waveformBufferCapacity = env->GetDirectBufferCapacity(forWaveform);
  assert(waveformBuffer);

  // Call into scom
  int bytesWritten = encodeMessage((char *)payloadBytes, payloadLength, waveformBuffer,
    waveformBufferCapacity);

  env->ReleaseByteArrayElements(payload, payloadBytes, 0);

  // Set bytebuffer limit
  jclass cls = env->GetObjectClass(forWaveform);
  jmethodID limitMethod = env->GetMethodID(cls, "limit", "(I)Ljava/nio/Buffer;");
  env->CallObjectMethod(forWaveform, limitMethod, bytesWritten);
  ll(LOG_INFO, "SCOM", "Buffer size written %d", bytesWritten);
}

void Java_com_example_scom_nativ_Jni_decodeAudio(JNIEnv *env, jobject thiz, jobject waveform) {
  char *waveformBuffer = (char *)env->GetDirectBufferAddress(waveform);
  jclass cls = env->GetObjectClass(waveform);
  jmethodID limitMethod = env->GetMethodID(cls, "limit", "()I");
  int waveformBytes = env->CallIntMethod(waveform, limitMethod);

  decodeAudio(waveformBuffer, waveformBytes);
}

jboolean Java_com_example_scom_nativ_Jni_messageAvailable(JNIEnv *env, jobject thiz) {
  return messageAvailable();
}

jint Java_com_example_scom_nativ_Jni_takeMessage(JNIEnv *env, jobject thiz, jbyteArray target) {
  jbyte *targetBytes = env->GetByteArrayElements(target, 0);
  int targetLen = env->GetArrayLength(target);

  int bytesWritten = takeMessage((char *)targetBytes, targetLen);

  env->ReleaseByteArrayElements(target, targetBytes, 0);
  return bytesWritten;
}
