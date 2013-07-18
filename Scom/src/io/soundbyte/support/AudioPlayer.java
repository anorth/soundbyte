package io.soundbyte.support;

import io.soundbyte.core.Engine;

import java.nio.ByteBuffer;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicReference;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

/**
 * Encodes and transmits messages via the Android audio hardware.
 * 
 * A message is sent repeatedly until replaced.
 */
public class AudioPlayer extends Thread implements ListeningPolicy {
  
  private static final String TAG = "SoundbyteAudioPlayer";
  private static final int AUDIO_ENCODING = AudioFormat.ENCODING_PCM_16BIT;
  // Millis of silence to play after each message.
  private static final int SILENCE_MS = 50;
  
  private final Engine engine;
  // Audio data to play. Bytes represent shorts, little-endian. The value may be null,
  // but not an empty array.
  private final AtomicReference<byte[]> playBuffer = new AtomicReference<byte[]>();
  private final AtomicLong numSamplesWritten = new AtomicLong();
  
  private AudioTrack tracker = null;
  private volatile CountDownLatch playLatch = new CountDownLatch(1);
  private volatile boolean stopped = false;
  private volatile boolean isWritingToTracker = false;
  
  /**
   * Creates a new player.
   * 
   * @param engine engine with which to encode messages
   */
  public AudioPlayer(Engine engine) {
    this.engine = engine;
  }
  
  @Override
  public void run() {
    android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO); 
    int numSilenceSamples = engine.sampleRate() * SILENCE_MS / 1000;
    byte[] silence = new byte[numSilenceSamples * engine.bytesPerSample()];
    try {
      int bufferSize = AudioTrack.getMinBufferSize(engine.sampleRate(),
          AudioFormat.CHANNEL_OUT_MONO, AUDIO_ENCODING);
      tracker = new AudioTrack(AudioManager.STREAM_MUSIC, engine.sampleRate(),
          AudioFormat.CHANNEL_OUT_MONO, AUDIO_ENCODING, bufferSize, AudioTrack.MODE_STREAM);
      if (tracker.getState() == AudioTrack.STATE_INITIALIZED) {
        Log.i(TAG, "New audio tracker initialised in state " + tracker.getPlayState());
        tracker.play();
        // This looks like a busy loop but it's not very busy as the thread quickly
        // blocks on playLatch.await() or tracker.write().
        while (!stopped) {
          playLatch.await();
          byte[] buffer = playBuffer.get();
          if (buffer != null) {
            int bufferSamples = buffer.length / engine.bytesPerSample();
            isWritingToTracker = true;
            try {
              Log.i(TAG, "Writing " + bufferSamples + " samples to player");
              tracker.write(buffer, 0, buffer.length);
              numSamplesWritten.addAndGet(bufferSamples);
              tracker.write(silence, 0, silence.length);
              numSamplesWritten.addAndGet(numSilenceSamples);
            } finally {
              isWritingToTracker = false;
            }
          }
        }
      } else {
        Log.e(TAG, "Couldn't initialise tracker");
      }
    } catch (InterruptedException e) {
      Log.e(TAG, "Interrupted", e);
      Thread.currentThread().interrupt();
      throw new RuntimeException(e);
    } catch (RuntimeException e) {
      Log.e(TAG, "Failed", e);
      throw e;
    } finally {
      Log.w(TAG, "AudioOut exiting");
      if (tracker != null) {
        tracker.stop();        
        tracker.release();
        tracker = null;
      }
    }
  }
  
  /**
   * {@inheritDoc}
   * 
   * Returns false if audio is currently being written to the tracker, or the tracker 
   * is currently playing audio. This prevents the local device receiving its own messages.
   */
  @Override
  public boolean canListenNow() {
    if (isWritingToTracker) {
      return false;
    }    
    return tracker.getPlaybackHeadPosition() >= numSamplesWritten.get();
  }

  /**
   * Schedules a message for sending when the previous message has finished. The message
   * is sent repeatedly, until replaced or set null/empty.
   * 
   * @param payload the message payload to send, or null.
   */
  public void sendMessage(byte[] payload) {
    byte[] data = null;
    if (payload != null && payload.length > 0) {
      ByteBuffer buffer = engine.encodeMessage(payload);
      data = new byte[buffer.remaining()];
      buffer.get(data);
    }
    assert (data == null) || data.length > 0;
    playBuffer.set(data);
    playLatch.countDown();
    if (data == null) {
      playLatch = new CountDownLatch(1);      
    }
  }
  
  /**
   * Signals this thread to stop after playing the current message.
   */
  public void close() {
    stopped = true;
  }
}
