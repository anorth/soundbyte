package io.soundbyte.core;

import java.nio.ByteBuffer;

public class Jni {

  static {
    System.loadLibrary("scomjni");
  }
  
  public synchronized native void init(int base, int subcarriers, int subcarrierSpacing, int chipRate);

  public synchronized native void encodeMessage(byte[] payload, ByteBuffer forWaveform);

  public synchronized native int decodeAudio(ByteBuffer audio);

  public synchronized native boolean messageAvailable();

  public synchronized native int takeMessage(byte[] target);
}
