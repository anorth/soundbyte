package io.soundbyte.tethered;

public interface SocketListener {
  void socketConnected(String name);
  void socketDisconnected(String name);

}
