package com.example.audioserver;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;

import android.util.Log;

class AudioServer extends Thread {

  public static final int MIC_PORT = 16000;
  public static final int Q_SIZE = 100;
  private static final String TAG = "AudioServer";

  private final BlockingQueue<byte[]> q = new ArrayBlockingQueue<byte[]>(100);
  private volatile Socket client = null;
  private volatile boolean running = true;

  @Override
  public void run() {
    ServerSocket server;
    try {
      server = new ServerSocket(MIC_PORT);
    } catch (IOException e) {
      throw new RuntimeException(e);
    }

    while (running) {
      // Wait for connection
      try {
        Log.i(TAG, "Awaiting connection on port " + MIC_PORT);
        client = server.accept();
        Log.i(TAG, "Received connection " + client.getInetAddress());
      } catch (IOException e) {
        Log.e(TAG, "Error accepting socket", e);
        throw new RuntimeException(e);
      }

      try {
        while (client != null && running) {
          Log.v(TAG, "Waiting for data, q has " + q.size());
          byte[] buffer = q.poll(1, TimeUnit.SECONDS);
          if (buffer != null) {
            Log.v(TAG, "Writing chunk of size " + buffer.length);
            client.getOutputStream().write(buffer);            
          } else {
            Log.w(TAG, "Timed out waiting for audio");
          }
        }
      } catch (InterruptedException e) {
        Log.e(TAG, "Interrupted, quitting");
        Thread.currentThread().interrupt();
        return;
      } catch (IOException e) {
        Log.e(TAG, "IO exception writing to client: " + e.getMessage());
      }

      if (client != null) {
        Log.i(TAG, "Closing client connection");
        try {
          client.close();
          client = null;
        } catch (IOException e) {
          Log.e(TAG, "IO exception closing client", e);
        }
      }
    }

    Log.i(TAG, "Shutting down server");
    try {
      server.close();
    } catch (IOException e) {
      Log.e(TAG, "Error closing server", e);
    }
    Log.w(TAG, "AudioServer thread exiting");
  }

  public void send(byte[] buffer) {
    if (client != null) {
      boolean ok = q.offer(buffer); // Might drop this buffer if queue is full
      if (!ok) {
        // If the buffer was full, clear it entirely so that subsequent writes
        // will be continuguous
        Log.i(TAG, "Queue was full when data offered, emptying");
        q.clear();
      }
      //Log.v(TAG, (ok ? "Received" : "Dropped") + " buffer");

    }
  }

  void close() {
    running = false;
  }
}