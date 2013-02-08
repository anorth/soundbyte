package com.example.scom.tethered;

import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;

import android.util.Log;

import com.example.scom.Events.SocketConnected;
import com.example.scom.Events.SocketDisconnected;
import com.squareup.otto.Bus;

class BufferedSocket extends Thread {

  public static final int Q_SIZE = 100;
  private static final String TAG = "BufferedSocket";

  private final String name;
  private final int port;
  private final Bus bus;
  private final BlockingQueue<byte[]> inputQ = new ArrayBlockingQueue<byte[]>(100);
  private volatile Socket client = null;
  private volatile boolean running = true;

  public BufferedSocket(String name, int port, Bus bus) {
    super("BufferedSocket:" + name);
    this.name = name;
    this.port = port;
    this.bus = bus;
  }

  @Override
  public void run() {
    try {
      ServerSocket server;
      try {
        server = new ServerSocket(port);
        server.setReuseAddress(true);
        server.setSoTimeout(2000);
      } catch (IOException e) {
        throw new RuntimeException(e);
      }

      while (running) {
        awaitConnection(server);
        //makeConnect();

        try {
          while (client != null && running) {
  //          Log.v(TAG, "[I] Waiting for data, q has " + inputQ.size());
            byte[] buffer = inputQ.poll(1, TimeUnit.SECONDS);
            if (buffer != null) {
  //            Log.v(TAG, "[I] Writing chunk of size " + buffer.length);
              client.getOutputStream().write(buffer);
            } else {
              Log.v(TAG, "[I] Timed out waiting for data to send");
            }
          }
        } catch (IOException e) {
          Log.e(TAG, "[I] IO exception writing to client: " + e.getMessage());
        } finally {
          closeSocket();
        }
      }

      Log.i(TAG, "Shutting down server");
      try {
        server.close();
      } catch (IOException e) {
        Log.e(TAG, "Error closing server", e);
      }
    } catch (InterruptedException e) {
      Log.e(TAG, "[I] Interrupted, quitting");
      Thread.currentThread().interrupt();
      return;
    } finally {
      Log.w(TAG, "Socket thread exiting");
    }
  }

  private void awaitConnection(ServerSocket server) {
    while (running && client == null) {
      try {
        Log.v(TAG, "Awaiting connection on port " + port);
        client = server.accept();
        Log.i(TAG, "Received connection " + client.getInetAddress() + ":" + port);
        bus.post(new SocketConnected(name));
      } catch (SocketTimeoutException e) {
        // Try again.
      } catch (IOException e) {
        Log.e(TAG, "Error accepting socket", e);
        throw new RuntimeException(e);
      }
    }
  }

  private void makeConnection() throws InterruptedException {
    while (true) {
      try {
        client = new Socket(InetAddress.getByAddress(null, new byte[] {
                /* 192 */-64,/* 168 */-88, 1, 108 }), port);
        break;
      } catch (UnknownHostException e) {
        throw new RuntimeException(e);
      } catch (IOException e) {
        Log.w(TAG, "[I] IO exception creating socket. Ensure other end is running! - "
                + e.getMessage());
        Thread.sleep(1000);
      }
    }
  }

  private void closeSocket() {
    if (client != null) {
      Log.i(TAG, "Closing client connection");
      try {
        client.close();
      } catch (IOException e) {
        Log.e(TAG, "IO exception closing client", e);
      } finally {
        client = null;
        bus.post(new SocketDisconnected(name));
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
    byte[] buf = new byte[maxBytes];
    while (client != null) {
//      Log.v(TAG, "[O] Receiving buffer of " + maxBytes);
      try {
        int bytesRead = client.getInputStream().read(buf);
        if (bytesRead > 0) {
          Log.v(TAG, "[O] Received " + bytesRead + " bytes");
          return Arrays.copyOf(buf, bytesRead);
        } else if (bytesRead == -1) {
          closeSocket();
        }
      } catch (IOException e) {
        if (running) {
          Log.e(TAG, "[O] Exception reading input socket", e);
        } else {
          Log.d(TAG, "[O] Exception reading input socket", e);
        }
      }
    }
    return new byte[0];
  }

  void close() {
    running = false;
  }
}
