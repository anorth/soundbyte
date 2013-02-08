package com.example.scom;

import java.nio.ByteBuffer;

import com.squareup.otto.Bus;

public class TetheredEngine implements Engine {

//  private static final String TAG = "TetheredEngine";
  
  private final BufferedSocket tetherSocket;

  public TetheredEngine(int port, Bus bus) {
    tetherSocket = new BufferedSocket("tether", port, bus);
  }
  
  @Override
  public void start() {
    tetherSocket.start();
  }
  
  @Override
  public void stop() {
    tetherSocket.close();
  }
  
  @Override
  public ByteBuffer encode(byte[] payload) {
    // FIXME send payload, receive audio
    return null;
  }

  @Override
  public void receiveAudio(ByteBuffer audio) {
//    Log.v(TAG, "Offboarding " + audio.limit() + " bytes of audio");
    byte[] array = audio.array();
    tetherSocket.send(array);
  }

  @Override
  public boolean messageAvailable() {
    return true; // but it might be empty
  }

  @Override
  public byte[] takeMessage() {
    return tetherSocket.receive(100);
  }

}
