package com.example.scom;

import android.util.Log;

import com.example.scom.Events.MessageReceived;
import com.squareup.otto.Bus;

class DataProcessor extends Thread {

  private static final String TAG = "DataProcessor";

  private final BufferedSocket server;
  private final Bus bus;

  private volatile boolean stopped = false;

  DataProcessor(BufferedSocket server, Bus bus) {
    super("DataProcessorThread");
    this.server = server;
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
      byte[] messageBuffer = new byte[1000];
      int msgBufferCount = 0;
      while (!stopped) {
        byte[] buffer = server.receive(100);
        if (buffer.length > 0) {
          Log.i(TAG, "Received data buffer of " + buffer.length + " bytes");
          for (int i = 0; i < buffer.length; ++i) {
            Log.d(TAG, String.format("%d", buffer[i]));
            if (buffer[i] == '\n') {
              handleMessage(new String(messageBuffer, 0, msgBufferCount));
              return;
            }
            messageBuffer[msgBufferCount++] = buffer[i];
          }
        } else {
          Log.d(TAG, "receiveBuffer returned empty buffer");
          Thread.sleep(1000);
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
