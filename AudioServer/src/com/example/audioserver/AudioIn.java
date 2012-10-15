package com.example.audioserver;

import java.util.Arrays;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder.AudioSource;
import android.util.Log;

class AudioIn extends Thread {

  private static final int BYTES_PER_SAMPLE = 2;
  private static final int BUF_SAMPLES = 441;
  private static final int N_BUFS = 100;
  private static final int SAMPLE_RATE = 44100;
  private static final String TAG = "AudioIn";
  
  private final AudioServer server;
  
  private volatile boolean stopped = false;
  
  AudioIn(AudioServer server) {
    this.server = server;
  }
  
  @Override
  public void run() {
    android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
    byte[][] buffers = new byte[N_BUFS][BUF_SAMPLES * BYTES_PER_SAMPLE];
    int bufIndex = 0;

    int minArInternalBufferSize = AudioRecord.getMinBufferSize(SAMPLE_RATE,
        AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
    Log.d(TAG, "Min AudioRecord internal buffer size = " + minArInternalBufferSize);
    
    AudioRecord recorder = null;
    try {
      recorder = new AudioRecord(AudioSource.MIC, SAMPLE_RATE, 
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
            Log.v(TAG, "Received audio buffer of " + (bytesRead / BYTES_PER_SAMPLE) + " samples");
            process(buffer, 0, bytesRead);
          } else {
            Log.e(TAG, "AudioRecord.read returned " + bytesRead);            
          }
        }
      } else {
        Log.e(TAG, "Couldn't initialise audio");
      }
    } catch (RuntimeException e) {
      Log.e(TAG, "Failed", e);
    } catch (Throwable t) {
      Log.e(TAG, "Failed", t);      
    } finally {
      Log.w(TAG, "AudioIn exiting");
      if (recorder != null) {
        recorder.stop();        
        recorder.release();
        recorder = null;
      }
    }
  }

  private void process(byte[] buffer, int offset, int length) {
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
