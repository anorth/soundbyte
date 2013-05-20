package io.soundbyte.app;

import java.nio.ByteBuffer;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder.AudioSource;
import android.util.Log;

class AudioListener extends Thread {

  private static final int BUF_SAMPLES = 441;
  private static final int N_BUFS = 100;
  private static final String TAG = "AudioIn";
  
  private final Engine engine;
  private final AudioPlayer player; // hack
  
  private volatile boolean stopped = false;
  
  AudioListener(Engine engine, AudioPlayer player) {
    this.engine = engine;
    this.player = player;
  }
 
  @Override
  public void run() {
    android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
    ByteBuffer[] buffers = new ByteBuffer[N_BUFS];
    for (int i = 0; i < buffers.length; ++i) {
      buffers[i] = ByteBuffer.allocateDirect(BUF_SAMPLES * Constants.BYTES_PER_SAMPLE);
    }

    int bufIndex = 0;

    int minArInternalBufferSize = AudioRecord.getMinBufferSize(Constants.SAMPLE_RATE,
        AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
    Log.d(TAG, "Min AudioRecord internal buffer size = " + minArInternalBufferSize);
    
    AudioRecord recorder = null;
    try {
      recorder = new AudioRecord(AudioSource.MIC, Constants.SAMPLE_RATE, 
          AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT, minArInternalBufferSize * 10);
      if (recorder.getState() == AudioRecord.STATE_INITIALIZED) {
        Log.i(TAG, "New audio recorder initialised " + recorder.getRecordingState());
        recorder.startRecording();
        Log.i(TAG, "Recording started: " + recorder.getRecordingState());
        while (!stopped) {
          ByteBuffer buffer = buffers[bufIndex++ % buffers.length];
//          Log.v(TAG, "Awaiting buffer, recording state " + recorder.getRecordingState());
          // Note: bytes represent shorts, little-endian
          int bytesRead = recorder.read(buffer, buffer.capacity());
          
          // hack: silence incoming audio if sending.
          if (player.isSending()) {
            buffer.rewind();
            buffer.put(new byte[bytesRead]);
            //Log.d(TAG, "Zeroing receive buffer due to concurrent send");
          }
              
          if (bytesRead > 0) {
//            Log.v(TAG, "Received audio buffer of " + (bytesRead / Constants.BYTES_PER_SAMPLE) + " samples");
            buffer.rewind();
            buffer.limit(bytesRead);
            sendBuffer(buffer);
          } else {
            Log.e(TAG, "AudioRecord.read returned " + bytesRead);
            Thread.sleep(1000);
          }
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
      Log.w(TAG, "AudioIn exiting");
      if (recorder != null) {
        recorder.stop();        
        recorder.release();
        recorder = null;
      }
    }
  }

  private void sendBuffer(ByteBuffer buffer) {
    engine.receiveAudio(buffer);
  }

  void close() {
    stopped = true;
  }
}
