package io.soundbyte.support;

import io.soundbyte.core.Engine;

import java.nio.ByteBuffer;
import java.util.Arrays;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder.AudioSource;
import android.util.Log;

/**
 * Records audio from the Android system and processes it with the Soundbyte engine,
 * providing messages when they are received.
 */
public class AudioListener extends Thread {

  private static final int BUF_SAMPLES = 441;
  private static final int N_BUFS = 100;
  private static final String TAG = "SoundbyteAudioListener";
  
  private final Engine engine;
  private final MessageReceiver messageReceiver;
  private final ListeningPolicy policy;

  private int lastProgress = 0;
  private volatile boolean stopped = false;
  
  public AudioListener(Engine engine, MessageReceiver receiver, ListeningPolicy policy) {
    this.engine = engine;
    this.messageReceiver = receiver;
    this.policy = policy;
  }
 
  @Override
  public void run() {
    android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
    ByteBuffer[] buffers = allocateBuffers();

    int bufIndex = 0;
    int minArInternalBufferSize = AudioRecord.getMinBufferSize(engine.sampleRate(),
        AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
    Log.d(TAG, "Min AudioRecord internal buffer size = " + minArInternalBufferSize);
    
    AudioRecord recorder = null;
    try {
      recorder = new AudioRecord(AudioSource.MIC, engine.sampleRate(), AudioFormat.CHANNEL_IN_MONO, 
          AudioFormat.ENCODING_PCM_16BIT, minArInternalBufferSize * 10);
      if (recorder.getState() == AudioRecord.STATE_INITIALIZED) {
        Log.d(TAG, "New audio recorder initialised in state " + recorder.getRecordingState());
        recorder.startRecording();
        Log.d(TAG, "Recording started, state " + recorder.getRecordingState());
        while (!stopped) {
          ByteBuffer buffer = buffers[bufIndex++ % buffers.length];
          receiveBuffer(recorder, buffer);
        }
      } else {
        Log.e(TAG, "Couldn't initialise audio");
      }
    } catch (RuntimeException e) {
      Log.e(TAG, "Failed", e);
      throw e;
    } catch (InterruptedException e) {
      Log.e(TAG, "Interrupted", e);
      Thread.currentThread().interrupt();
      throw new RuntimeException(e);
    } finally {
      Log.w(TAG, "AudioListener exiting");
      if (recorder != null) {
        recorder.stop();        
        recorder.release();
        recorder = null;
      }
    }
  }

  public void close() {
    stopped = true;
  }

  private ByteBuffer[] allocateBuffers() {
    ByteBuffer[] buffers = new ByteBuffer[N_BUFS];
    for (int i = 0; i < buffers.length; ++i) {
      buffers[i] = ByteBuffer.allocateDirect(BUF_SAMPLES * engine.bytesPerSample());
    }
    return buffers;
  }

  private void receiveBuffer(AudioRecord recorder, ByteBuffer buffer) throws InterruptedException {
    // Log.v(TAG, "Awaiting buffer, recording state " + recorder.getRecordingState());
    // Note: bytes represent shorts, little-endian
    int bytesRead = recorder.read(buffer, buffer.capacity());
    if (!policy.canListenNow()) {
      // Replace recorded audio with silence.
      buffer.rewind();
      buffer.put(new byte[bytesRead]);
    }

    if (bytesRead > 0) {
      // Log.v(TAG, "Received audio buffer of " + (bytesRead / Constants.BYTES_PER_SAMPLE)
      // + " samples");
      buffer.rewind();
      buffer.limit(bytesRead);
      processBuffer(buffer);
    } else {
      Log.w(TAG, "AudioRecord.read returned " + bytesRead + " bytes, expected > 0");
      Thread.sleep(100);
    }
  }

  private void processBuffer(ByteBuffer buffer) {
    engine.receiveAudio(buffer);
    int progress = engine.messageProgress();
    if (progress != lastProgress) {
      messageReceiver.receiveProgress(progress);
      lastProgress = progress;
    }
    if (engine.messageAvailable()) {
      byte[] msg = engine.takeMessage();
      Log.i(TAG, "Received data buffer of " + msg.length + " bytes: " + Arrays.toString(msg));
      if (messageReceiver != null && msg.length > 0) {
        messageReceiver.receiveMessage(msg);
      }
    }
  }
}
