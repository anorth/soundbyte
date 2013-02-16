package com.example.scom.nativ;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;

import android.util.Log;

import com.example.scom.Constants;
import com.example.scom.Engine;

public class NativeEngine implements Engine {

  private static final String TAG = "NativeEngine";
  private static final int MAX_MESSAGE_LENGTH = 250;
  
  private final Jni jni;
  private final BlockingQueue<ByteBuffer> waveforms = new ArrayBlockingQueue<ByteBuffer>(100);

  public NativeEngine() {
    jni = new Jni();
    jni.init(
        14000, //base
        50,    //rate
        2,     //spacing 
        8      //channels
        );
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
    
    // TODO: don't do work in this thread.
    jni.encodeMessage(payload, forWaveform);
    
    Log.i(TAG, "Encoded returned " + forWaveform.limit() + " " + forWaveform.remaining());
    waveforms.add(forWaveform);
  }

  @Override
  public boolean audioAvailable() {
    return !waveforms.isEmpty();
  }

  @Override
  public ByteBuffer takeAudio() {
    Log.e(TAG, "native.takeAudio()");
    
    try {
      ByteBuffer result = waveforms.take();
      Log.e(TAG, "native.takeAudio() TAKEN");
      return result;
    } catch (InterruptedException e) {
      Log.e(TAG, "native.takeAudio() INTERRUPTED");
      Thread.currentThread().interrupt();
      throw new RuntimeException(e);
    }
  }

  @Override
  public void receiveAudio(ByteBuffer audio) {
    jni.decodeAudio(audio);
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

  private static ByteBuffer allocateWaveformBuffer() {
    int nbytes = Constants.SAMPLE_RATE * Constants.BYTES_PER_SAMPLE * 10; // max 10s
    return ByteBuffer.allocateDirect(nbytes);
  }
}
