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
  
  final class MessageReceived {
    public final String msg;
    public MessageReceived(String msg) {
      this.msg = msg;
    }
  }
  
  final class MessageProgress {
    public final int progress;
    public MessageProgress(int progress) {
      this.progress = progress;
    }
  }
}
