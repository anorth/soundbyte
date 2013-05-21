package io.soundbyte.support;

/**
 * Receives decoded messages and progress indications.
 */
public interface MessageReceiver {
  /**
   * Receive an integer progress indication, positive if a message has been
   * partially decoded. Progress numbers increase as a message is sequentially
   * received and decoded, and fall back to zero if decoding fails.
   */
  void receiveProgress(int progress);

  /**
   * Receives a complete decoded message.
   */
  void receiveMessage(byte[] message);
}
