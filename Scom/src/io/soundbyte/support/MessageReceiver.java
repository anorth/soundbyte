package io.soundbyte.support;

public interface MessageReceiver {
  void receiveProgress(int progress);
  void receiveMessage(byte[] message);
}
