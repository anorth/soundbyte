package io.soundbyte.app;

import io.soundbyte.app.Events.MessageProgress;
import io.soundbyte.app.Events.MessageReceived;
import io.soundbyte.core.Engine;

import java.util.Arrays;

import android.util.Log;

import com.squareup.otto.Bus;

class DataProcessor extends Thread {

  private static final String TAG = "DataProcessor";

  private final Engine engine;
  private final Bus bus;

  private volatile boolean stopped = false;
  
  private int lastProgress = -1;

  DataProcessor(Engine engine, Bus bus) {
    super("DataProcessorThread");
    this.engine = engine;
    this.bus = bus;
  }

  @Override
  public void run() {
    try {
      while (!stopped) {
        receiveMessage();
      }
    } catch (RuntimeException e) {
      Log.e(TAG, "Failed", e);
      throw e;
    } finally {
      Log.w(TAG, "Exiting");
    }
  }

  private void receiveMessage() {
    try {
      while (!stopped) {
        handleProgress(engine.messageProgress());

        // TODO: rather than busy loop, it would be better to check only
        // after audio has been received
        if (engine.messageAvailable()) {
          byte[] buffer = engine.takeMessage();
          if (buffer.length > 0) {
            Log.i(TAG, "Received data buffer of " + buffer.length + " bytes: " + Arrays.toString(buffer));
            handleMessage(new String(buffer));
          } else {
            Log.d(TAG, "receiveBuffer returned empty buffer");
          }
          Thread.sleep(100);
        }
      }
    } catch (InterruptedException e) {
      Log.e(TAG, "Interrupted", e);
      Thread.currentThread().interrupt();
      throw new RuntimeException(e);
    }
  }

  private void handleMessage(String msg) {
    Log.e(TAG, "MESSAGE: " + msg);
    bus.post(new MessageReceived(msg));
  }

  private void handleProgress(int progress) {
    if (progress != lastProgress) {
      lastProgress = progress;
      Log.i(TAG, "Progress: " + progress);
      bus.post(new MessageProgress(progress));
    }
  }

  void close() {
    stopped = true;
  }
}
