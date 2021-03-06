#include <jni.h>
#include <string.h>
#include <assert.h>

#include "scom.h"
#include "log.h"

// Export definitions for JNI
extern "C" {
  void Java_io_soundbyte_core_Jni_init(JNIEnv* env, jobject thiz,
      jint base, jint subcarriers, jint subcarrierSpacing, jint chipRate);

  void Java_io_soundbyte_core_Jni_encodeMessage(JNIEnv *env, jobject thiz, jbyteArray payload,
      jobject forWaveform);

  jint Java_io_soundbyte_core_Jni_decodeAudio(JNIEnv *env, jobject thiz, jobject buffer);

  jboolean Java_io_soundbyte_core_Jni_messageAvailable(JNIEnv *env, jobject thiz);

  jint Java_io_soundbyte_core_Jni_takeMessage(JNIEnv *env, jobject thiz, jbyteArray target);
}

// Implementation

void Java_io_soundbyte_core_Jni_init(JNIEnv* env, jobject thiz,
    jint base, jint subcarriers, jint subcarrierSpacing, jint chipRate) {
  scomInit(base, chipRate, subcarrierSpacing, subcarriers);
}

void Java_io_soundbyte_core_Jni_encodeMessage(JNIEnv *env, jobject thiz, jbyteArray payload,
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

jint Java_io_soundbyte_core_Jni_decodeAudio(JNIEnv *env, jobject thiz, jobject waveform) {
  char *waveformBuffer = (char *)env->GetDirectBufferAddress(waveform);
  jclass cls = env->GetObjectClass(waveform);
  jmethodID limitMethod = env->GetMethodID(cls, "limit", "()I");
  int waveformBytes = env->CallIntMethod(waveform, limitMethod);

  return decodeAudio(waveformBuffer, waveformBytes);
}

jboolean Java_io_soundbyte_core_Jni_messageAvailable(JNIEnv *env, jobject thiz) {
  return messageAvailable();
}

jint Java_io_soundbyte_core_Jni_takeMessage(JNIEnv *env, jobject thiz, jbyteArray target) {
  jbyte *targetBytes = env->GetByteArrayElements(target, 0);
  int targetLen = env->GetArrayLength(target);

  int bytesWritten = takeMessage((char *)targetBytes, targetLen);

  env->ReleaseByteArrayElements(target, targetBytes, 0);
  return bytesWritten;
}
