package com.example.scom;

import java.util.Arrays;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder.AudioSource;
import android.util.Log;

class AudioIn extends Thread {

  private static final int BUF_SAMPLES = 441;
  private static final int N_BUFS = 100;
  private static final String TAG = "AudioIn";
  
  private final BufferedSocket server;
  
  private volatile boolean stopped = false;
  
  AudioIn(BufferedSocket server) {
    this.server = server;
  }
  
  @Override
  public void run() {
    android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
    byte[][] buffers = new byte[N_BUFS][BUF_SAMPLES * Constants.BYTES_PER_SAMPLE];
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
          byte[] buffer = buffers[bufIndex++ % buffers.length];
          Log.v(TAG, "Awaiting buffer, recording state " + recorder.getRecordingState());
          // Note: bytes represent shorts, little-endian
          int bytesRead = recorder.read(buffer, 0, buffer.length);
          if (bytesRead > 0) {
            Log.v(TAG, "Received audio buffer of " + (bytesRead / Constants.BYTES_PER_SAMPLE) + " samples");
            sendBuffer(buffer, 0, bytesRead);
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

  private void sendBuffer(byte[] buffer, int offset, int length) {
    byte[] trimmed = buffer;
    if (buffer.length != length) {
      trimmed = Arrays.copyOfRange(buffer, offset, offset + length);
    }
    server.send(trimmed);
  }

  void close() {
    stopped = true;
  }
}
