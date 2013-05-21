package io.soundbyte.core;

import java.nio.ByteBuffer;
import java.util.Arrays;

import android.util.Log;

public class NativeEngine implements Engine {

  private static final String TAG = "NativeEngine";
  private static final int MAX_MESSAGE_LENGTH = 250;
  private static final int BUFFER_DURATION_SECONDS = 10;
  
  private final Jni jni;
  private volatile int receiveProgress;

  public static EngineConfiguration defaultConfiguration() {
    return new EngineConfiguration(18000, 8, 2, 50);
  }
  
  public NativeEngine() {
    this(defaultConfiguration());
  }
  
  public NativeEngine(EngineConfiguration config) {
    jni = new Jni();
    jni.init(config.baseFrequency, config.subcarriers, config.subcarrierSpacing, 
        config.chipRate);
  }
  
  @Override
  public int bytesPerSample() {
    return Constants.BYTES_PER_SAMPLE;
  }

  @Override
  public int sampleRate() {
    return Constants.SAMPLE_RATE;
  }

  @Override
  public void start() {
    Log.i(TAG, "Native engine starting");
  }

  @Override
  public synchronized void stop() {
  }

  @Override
  public ByteBuffer encodeMessage(byte[] payload) {
    if (payload == null) {
      return null;
    }
    
    ByteBuffer forWaveform = allocateWaveformBuffer();
    jni.encodeMessage(payload, forWaveform);
    Log.i(TAG, "Encoded " + payload.length + " byte payload into " + forWaveform.limit()
        + " bytes,  " + forWaveform.remaining() + " buffer bytes unused.");
    return forWaveform;
  }

  @Override
  public void receiveAudio(ByteBuffer audio) {
    receiveProgress = jni.decodeAudio(audio);
  }

  @Override
  public boolean messageAvailable() {
    return jni.messageAvailable();
  }

  @Override
  public byte[] takeMessage() {
    byte[] target = new byte[MAX_MESSAGE_LENGTH];
    int bytes = jni.takeMessage(target);
    if (bytes > 0) {
      return Arrays.copyOf(target, bytes);
    } else if (bytes == 0) {
      throw new RuntimeException("Message too long");
    } else {
      throw new IllegalStateException("No message available");
    }
  }
  
  @Override
  public int messageProgress() {
    return receiveProgress;
  }

  private ByteBuffer allocateWaveformBuffer() {
    int nbytes = sampleRate() * bytesPerSample() * BUFFER_DURATION_SECONDS;
    return ByteBuffer.allocateDirect(nbytes);
  }
}
