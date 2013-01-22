package com.example.audioserver;

import android.util.Log;

import com.example.audioserver.Events.SocketConnected;
import com.example.audioserver.Events.SocketDisconnected;
import com.squareup.otto.Bus;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.Arrays;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;

class AudioSocket extends Thread {

  public static final int PORT = 16002;
  public static final int Q_SIZE = 100;
  private static final String TAG = "AudioSocket";

  private final Bus bus;
  private final BlockingQueue<byte[]> inputQ = new ArrayBlockingQueue<byte[]>(100);
  private volatile Socket client = null;
  private volatile boolean running = true;

  public AudioSocket(Bus bus) {
    this.bus = bus;
  }

  @Override
  public void run() {
    ServerSocket server;
    try {
      server = new ServerSocket(PORT);
      server.setReuseAddress(true);
      server.setSoTimeout(2000);
    } catch (IOException e) {
      throw new RuntimeException(e);
    }

    while (running) {
      awaitConnection(server);

      try {
        while (client != null && running) {
          Log.v(TAG, "[I] Waiting for data, q has " + inputQ.size());
          byte[] buffer = inputQ.poll(1, TimeUnit.SECONDS);
          if (buffer != null) {
            Log.v(TAG, "[I] Writing chunk of size " + buffer.length);
            client.getOutputStream().write(buffer);
          } else {
            Log.w(TAG, "[I] Timed out waiting for audio");
          }
        }
      } catch (InterruptedException e) {
        Log.e(TAG, "[I] Interrupted, quitting");
        Thread.currentThread().interrupt();
        return;
      } catch (IOException e) {
        Log.e(TAG, "[I] IO exception writing to client: " + e.getMessage());
      }

      if (client != null) {
        Log.i(TAG, "Closing client connection");
        try {
          client.close();
          client = null;
          bus.post(new SocketDisconnected());
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

  private void awaitConnection(ServerSocket server) {
    while (running && client == null) {
      try {
        Log.i(TAG, "Awaiting connection on port " + PORT);
        client = server.accept();
        Log.i(TAG, "Received connection " + client.getInetAddress());
        bus.post(new SocketConnected());
      } catch (SocketTimeoutException e) {
        // Try again.
      } catch (IOException e) {
        Log.e(TAG, "Error accepting socket", e);
        throw new RuntimeException(e);
      }
    }
  }

  public void send(byte[] buffer) {
    if (client != null) {
      boolean ok = inputQ.offer(buffer); // Might drop this buffer if queue is full
      if (!ok) {
        // If the buffer was full, clear it entirely so that subsequent writes
        // will be contiguous
        Log.i(TAG, "Queue was full when data offered, emptying");
        inputQ.clear();
      }
      //Log.v(TAG, (ok ? "Received" : "Dropped") + " buffer");
    }
  }

  /** Reads at most maxBytes from the socket. */
  public byte[] receive(int maxBytes) {
    if (client != null) {
      Log.v(TAG, "[O] Receiving buffer of " + maxBytes);
      byte[] buf = new byte[maxBytes];
      try {
        int bytesRead = client.getInputStream().read(buf);
        Log.v(TAG, "[O] Received " + bytesRead + " bytes");
        if (bytesRead > 0) {
          return Arrays.copyOf(buf, bytesRead);
        }
      } catch (IOException e) {
        Log.e(TAG, "[O] Exception reading input socket", e);
      }
    }
    Log.v(TAG, "[O] Returning empty buffer");
    return new byte[0];
  }

  void close() {
    running = false;
  }
}
