package com.example.scom.nativ;

import java.nio.ByteBuffer;

public class Jni {

  static {
    System.loadLibrary("scomjni");
  }

  public native String stringFromJNI();
  
  public native void init();

  public native ByteBuffer encodeMessage(byte[] payload, ByteBuffer forWaveform);

  public native void decodeAudio(ByteBuffer audio);

  public native boolean messageAvailable();

  public native int takeMessage(byte[] target);
}
