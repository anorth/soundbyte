package io.soundbyte.app;

public interface Events {
  final class SocketConnected {
    public final String name;
    public SocketConnected(String name) {
      this.name = name;
    }
  };

  final class SocketDisconnected {
    public final String name;
    public SocketDisconnected(String name) {
      this.name = name;
    }
  };
}
