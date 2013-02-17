package com.example.scom.tethered;

import java.nio.ByteBuffer;

import android.util.Log;

import com.example.scom.Engine;
import com.squareup.otto.Bus;

public class TetheredEngine implements Engine {

  private static final String TAG = "TetheredEngine";
  
  // Sends audio, receives messages.
  private final BufferedSocket decodingSocket;
  // Sends messages, receives audio.
  private final BufferedSocket encodingSocket;
  
  public TetheredEngine(int port, Bus bus) {
    decodingSocket = new BufferedSocket("decoding", port, bus);
    encodingSocket = new BufferedSocket("encoding", port + 1, bus);
  }
  
  @Override
  public void start() {
    Log.i(TAG, "Tethered engine starting");
    decodingSocket.start();
    encodingSocket.start();
  }
  
  @Override
  public void stop() {
    decodingSocket.close();
    encodingSocket.close();
  }
  
  @Override
  public void receiveMessage(byte[] payload) {
    encodingSocket.send(payload);
  }
  
  @Override
  public boolean audioAvailable() {
    return true; // but it might be empty
  }
  
  @Override
  public ByteBuffer takeAudio() {
    byte[] received = encodingSocket.receive(10000);
    return ByteBuffer.wrap(received);
  }
  
  @Override
  public void receiveAudio(ByteBuffer audio) {
//    Log.v(TAG, "Offboarding " + audio.limit() + " bytes of audio");
    byte[] array = audio.array();
    decodingSocket.send(array);
  }

  @Override
  public boolean messageAvailable() {
    return true; // but it might be empty
  }

  @Override
  public byte[] takeMessage() {
    return decodingSocket.receive(100);
  }

  @Override
  public int messageProgress() {
    return 0;
  }
}
