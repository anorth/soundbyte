package io.soundbyte.core;

import java.nio.ByteBuffer;
import java.util.Arrays;

import android.util.Log;

public class NativeEngine implements Engine {

  private static final String TAG = "NativeEngine";
  private static final int MAX_MESSAGE_LENGTH = 250;
  private static final int BUFFER_DURATION_SECONDS = 10;
  
  private final Jni jni;
  //private final BlockingQueue<ByteBuffer> waveforms = new ArrayBlockingQueue<ByteBuffer>(100);
  private volatile ByteBuffer waveform = null;
  private volatile long lastSent = 0;
  private volatile int progress;

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
    waveform = null;
  }

  @Override
  // TODO(alex): Remove synchronization here in favour of JNI.
  public synchronized void receiveMessage(byte[] payload) {
    if (payload == null) {
      waveform = null;
      return;
    }
    
    ByteBuffer forWaveform = allocateWaveformBuffer();
    
    // TODO: don't do work in this thread.
    jni.encodeMessage(payload, forWaveform);
    
    Log.i(TAG, "Encoded returned " + forWaveform.limit() + " " + forWaveform.remaining());
    //waveforms.add(forWaveform);
    waveform = forWaveform;
  }

  @Override
  // TODO(alex): Remove synchronization here
  public synchronized boolean audioAvailable() {
    return waveform != null && millisUntilReady() <= 0; //!waveforms.isEmpty();
  }
  
  private long millisUntilReady() {
    if (waveform == null) return 0;
    waveform.rewind();
    return Math.max(0,
        lastSent + 50 // leave a 50ms gap 
        + (waveform.remaining() * 1000 / Constants.SAMPLE_RATE)
        - System.currentTimeMillis());
  }

  @Override
  // TODO(alex): Remove synchronization here
  public synchronized ByteBuffer takeAudio() {
    Log.e(TAG, "native.takeAudio()");
    
//    try {
    ByteBuffer result = waveform; //waveforms.take();
    waveform.rewind();
    lastSent = System.currentTimeMillis();
    Log.e(TAG, "native.takeAudio() TAKEN");
    return result;
//    } catch (InterruptedException e) {
//      Log.e(TAG, "native.takeAudio() INTERRUPTED");
//      Thread.currentThread().interrupt();
//      throw new RuntimeException(e);
//    }
  }

  @Override
  public void receiveAudio(ByteBuffer audio) {
    progress = jni.decodeAudio(audio);
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
    return progress;
  }

  private ByteBuffer allocateWaveformBuffer() {
    int nbytes = sampleRate() * bytesPerSample() * BUFFER_DURATION_SECONDS;
    return ByteBuffer.allocateDirect(nbytes);
  }
}
