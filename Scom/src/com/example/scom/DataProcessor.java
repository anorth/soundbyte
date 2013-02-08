package com.example.scom;

import java.util.Arrays;

import android.util.Log;

import com.example.scom.Events.MessageReceived;
import com.squareup.otto.Bus;

class DataProcessor extends Thread {

  private static final String TAG = "DataProcessor";

  private final Engine engine;
  private final Bus bus;

  private volatile boolean stopped = false;

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
        if (engine.messageAvailable()) {
          byte[] buffer = engine.takeMessage();
          if (buffer.length > 0) {
            Log.i(TAG, "Received data buffer of " + buffer.length + " bytes: " + Arrays.toString(buffer));
            handleMessage(new String(buffer));
          } else {
            Log.d(TAG, "receiveBuffer returned empty buffer");
            Thread.sleep(1000);
          }
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

  void close() {
    stopped = true;
  }
}
