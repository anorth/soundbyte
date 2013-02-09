package com.example.scom.nativ;

import java.nio.ByteBuffer;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;

import android.util.Log;

import com.example.scom.Constants;
import com.example.scom.Engine;

public class NativeEngine implements Engine {

  private static final String TAG = "NativeEngine";
  
  private final Jni jni;
  private final BlockingQueue<ByteBuffer> waveforms = new ArrayBlockingQueue<ByteBuffer>(100);

  public NativeEngine() {
    jni = new Jni();
  }
  
  @Override
  public void start() {
    Log.i(TAG, "Native engine starting: " + jni.stringFromJNI());
  }

  @Override
  public void stop() {
  }

  @Override
  public void receiveMessage(byte[] payload) {
    ByteBuffer forWaveform = allocateWaveformBuffer();
    jni.encodeMessage(payload, forWaveform);
    waveforms.add(forWaveform);
  }

  @Override
  public boolean audioAvailable() {
    return !waveforms.isEmpty();
  }

  @Override
  public ByteBuffer takeAudio() {
    try {
      return waveforms.take();
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
      throw new RuntimeException(e);
    }
  }

  @Override
  public void receiveAudio(ByteBuffer audio) {
    // TODO Auto-generated method stub
    jni.decodeAudio(audio);
  }

  @Override
  public boolean messageAvailable() {
    return jni.messageAvailable();
  }

  @Override
  public byte[] takeMessage() {
    return jni.takeMessage();
  }

  private static ByteBuffer allocateWaveformBuffer() {
    int nbytes = Constants.SAMPLE_RATE * Constants.BYTES_PER_SAMPLE * 10; // max 10s
    return ByteBuffer.allocate(nbytes);
  }
}
