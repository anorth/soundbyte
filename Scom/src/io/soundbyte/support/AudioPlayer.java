package io.soundbyte.support;

import io.soundbyte.core.Engine;

import java.nio.ByteBuffer;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

public class AudioPlayer extends Thread implements ListeningPolicy {
  
  private static final String TAG = "AudioPlayer";
  private static final int AUDIO_ENCODING = AudioFormat.ENCODING_PCM_16BIT;
  
  private final Engine engine;
  
  private volatile boolean stopped = false;
  private volatile boolean isWritingToTracker = false;
  // NOTE(alex): This might be better achieved with a playback position marker.
  private volatile long playbackFinishMs = -1;
  
  public AudioPlayer(Engine engine) {
    this.engine = engine;
  }
  
  @Override
  public void run() {
    android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO); 
    
    AudioTrack tracker = null;
    try {
      int bufferSize = AudioTrack.getMinBufferSize(engine.sampleRate(),
          AudioFormat.CHANNEL_OUT_MONO, AUDIO_ENCODING);
      tracker = new AudioTrack(AudioManager.STREAM_MUSIC, engine.sampleRate(),
          AudioFormat.CHANNEL_OUT_MONO, AUDIO_ENCODING, bufferSize, AudioTrack.MODE_STREAM);
      if (tracker.getState() == AudioTrack.STATE_INITIALIZED) {
        Log.i(TAG, "New audio tracker initialised " + tracker.getPlayState());
        tracker.play();
        // TODO(alex): remove this busy loop
        while (!stopped) {
          byte[] buffer = engine.audioAvailable() ? receiveBuffer() : null;
          // Note: bytes represent shorts, little-endian
          if (buffer != null && buffer.length > 0) {
            int samples = buffer.length / engine.bytesPerSample();
            Log.v(TAG, "Received audio buffer of " + samples + " samples");
            int sendMillis = (samples * 1000 / engine.sampleRate());
            isWritingToTracker = true;
            try {
              Log.i(TAG, "Sending");
              tracker.write(buffer, 0, buffer.length);
              playbackFinishMs = System.currentTimeMillis() + sendMillis;
            } finally {
              isWritingToTracker = false;
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
  
  @Override
  public boolean canListenNow() {
    if (isWritingToTracker) {
      return false;
    }
    
    long now = System.currentTimeMillis();
    boolean isSending = playbackFinishMs >= now;
    return !isSending;
  }

  private byte[] receiveBuffer() {
    ByteBuffer buffer = engine.takeAudio();
    byte[] data = new byte[buffer.remaining()];
    buffer.get(data);
    return data;
  }

  public void close() {
    stopped = true;
  }
}
