package com.example.audioserver;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

class AudioOut extends Thread {
  
  private static final int BUFFER_SIZE = AudioTrack.getMinBufferSize(Constants.SAMPLE_RATE,
      AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);
  private static final String TAG = "AudioOut";
  
  private final AudioSocket server;
  
  private volatile boolean stopped = false;
  
  AudioOut(AudioSocket server) {
    this.server = server;
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
          byte[] buffer = receiveBuffer();
          // Note: bytes represent shorts, little-endian
          if (buffer.length > 0) {
            Log.v(TAG, "Received audio buffer of " + (buffer.length / Constants.BYTES_PER_SAMPLE) + " samples");
            tracker.write(buffer, 0, buffer.length);
          } else {
            Log.w(TAG, "receiveBuffer returned empty buffer");
            Thread.sleep(1000);
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

  private byte[] receiveBuffer() {
    return server.receive(BUFFER_SIZE);
  }

  void close() {
    stopped = true;
  }
}
