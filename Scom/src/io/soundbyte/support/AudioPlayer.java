package io.soundbyte.support;

import io.soundbyte.core.Engine;

import java.nio.ByteBuffer;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicReference;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

public class AudioPlayer extends Thread implements ListeningPolicy {
  
  private static final String TAG = "AudioPlayer";
  private static final int AUDIO_ENCODING = AudioFormat.ENCODING_PCM_16BIT;
  
  private final Engine engine;
  // Audio data to play. Bytes represent shorts, little-endian.
  private final AtomicReference<byte[]> playBuffer = new AtomicReference<byte[]>();
  private final AtomicLong numSamplesWritten = new AtomicLong();
  
  private AudioTrack tracker = null;
  private volatile boolean stopped = false;
  private volatile boolean isWritingToTracker = false;
  
  public AudioPlayer(Engine engine) {
    this.engine = engine;
  }
  
  @Override
  public void run() {
    android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO); 
    
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
          byte[] buffer = playBuffer.get();
          if (buffer != null && buffer.length > 0) {
            // TODO(alex): add a short break between consecutive messages
            // to minimise interference. Perhaps with a playback head marker?
            int bufferSamples = buffer.length / engine.bytesPerSample();
            Log.v(TAG, "Received audio buffer of " + bufferSamples + " samples");
            isWritingToTracker = true;
            try {
              Log.i(TAG, "Sending");
              tracker.write(buffer, 0, buffer.length);
              numSamplesWritten.addAndGet(bufferSamples);
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
    return tracker.getPlaybackHeadPosition() >= numSamplesWritten.get();
  }

  /**
   * Schedules a message for sending when the previous message has finished. The message
   * is sent repeatedly, until replaced or set null.
   */
  public void sendMessage(byte[] payload) {
    ByteBuffer buffer = engine.encodeMessage(payload);
    byte[] data = new byte[buffer.remaining()];
    buffer.get(data);
    playBuffer.set(data);
  }
  
  public void close() {
    stopped = true;
  }
}
