package com.example.scom;

import java.nio.ByteBuffer;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

class AudioPlayer extends Thread {
  
  private static final int BUFFER_SIZE = AudioTrack.getMinBufferSize(Constants.SAMPLE_RATE,
      AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);
  private static final String TAG = "AudioOut";
  
  private final Engine engine;
  
  private volatile boolean stopped = false;
  private volatile boolean isWriting = false;
  private volatile long sendFinishAt = -1;
  
  AudioPlayer(Engine engine) {
    this.engine = engine;
  }
  
  @Override
  public void run() {
    android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO); 
    
    AudioTrack tracker = null;
    try {
      tracker = new AudioTrack(AudioManager.STREAM_MUSIC, Constants.SAMPLE_RATE,
          AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT, BUFFER_SIZE,
          AudioTrack.MODE_STREAM);
      if (tracker.getState() == AudioTrack.STATE_INITIALIZED) {
        Log.i(TAG, "New audio tracker initialised " + tracker.getPlayState());
        tracker.play();
        while (!stopped) {
          Log.v(TAG, "Awaiting buffer, state " + tracker.getPlayState());
          byte[] buffer = engine.audioAvailable() ? receiveBuffer() : null;
          // Note: bytes represent shorts, little-endian
          if (buffer != null && buffer.length > 0) {
            int samples = buffer.length / Constants.BYTES_PER_SAMPLE;
            Log.v(TAG, "Received audio buffer of " + samples + " samples");
            int sendMillis = (samples * 1000 / Constants.SAMPLE_RATE);
            isWriting = true;
            try {
              Log.i(TAG, "Sending");
              tracker.write(buffer, 0, buffer.length);
              sendFinishAt = System.currentTimeMillis() + sendMillis;
            } finally {
              isWriting = false;
            }
          } else {
//            Log.v(TAG, "receiveBuffer returned empty buffer");
            Thread.sleep(200);
          }
        }
      } else {
        Log.e(TAG, "Couldn't initialise tracker");
      }
    } catch (RuntimeException e) {
      Log.e(TAG, "Failed", e);
      throw e;
    } catch (InterruptedException e) {
      Log.e(TAG, "Interrupted", e);
      Thread.currentThread().interrupt();
      throw new RuntimeException(e);
    } finally {
      Log.w(TAG, "AudioOut exiting");
      if (tracker != null) {
        tracker.stop();        
        tracker.release();
        tracker = null;
      }
    }
  }
  
  public boolean isSending() {
    if (isWriting) {
      return true;
    }
    
    // TODO: get some event when sending actually finished instead?
    long now = System.currentTimeMillis();
    boolean isSending = sendFinishAt >= now;
    return isSending;
  }

  private byte[] receiveBuffer() {
    ByteBuffer buffer = engine.takeAudio();
    byte[] data = new byte[buffer.remaining()];
    buffer.get(data);
    return data;
  }

  void close() {
    stopped = true;
  }
}
