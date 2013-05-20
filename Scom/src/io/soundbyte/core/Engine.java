package io.soundbyte.core;

import java.nio.ByteBuffer;

/**
 * Encodes and decodes data to/from audio buffers.
 */
public interface Engine {

  /** The number of bytes representing an audio sample (usually 2). */
  int bytesPerSample();
  
  /** The number of samples per second of signals this engine processes (usually 44.1k) */
  int sampleRate();
  
  /** Initialises engine internals. */
  void start();
  
  /** Shuts down internals. */
  void stop();
  
  /** Receives and (possibly synchronously) processes a message payload. */
  public void receiveMessage(byte[] payload);
  
  /** Whether audio data is available for playing. */
  public boolean audioAvailable();
  
  /** Dequeues encoded audio. */
  public ByteBuffer takeAudio();
  
  /** 
   * Receives and (possibly synchronously) processes a segment of audio data. 
   */
  void receiveAudio(ByteBuffer audio);
  
  /** Whether a message has been decoded. */
  boolean messageAvailable();
  
  /** Progress indicator (0 - nothing/finished, positive - some progress) */
  int messageProgress();
  
  /** Dequeues a decoded message. */
  byte[] takeMessage();	
}
