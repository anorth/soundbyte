package io.soundbyte.core;

import java.nio.ByteBuffer;

public class Jni {

  static {
    System.loadLibrary("scomjni");
  }

  public native String stringFromJNI();
  
  public native void init(int base, int subcarriers, int subcarrierSpacing, int chipRate);

  public native void encodeMessage(byte[] payload, ByteBuffer forWaveform);

  public native int decodeAudio(ByteBuffer audio);

  public native boolean messageAvailable();

  public native int takeMessage(byte[] target);
}
