package com.example.scom;

import java.nio.ByteBuffer;

/**
 * Encodes and decodes data to/from audio buffers.
 */
public interface Engine {

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
  
  /** Receives and (possibly synchronously) processes a segment of audio data. */
  void receiveAudio(ByteBuffer audio);
  
  /** Whether a message has been decoded. */
  boolean messageAvailable();
  
  /** Dequeues a decoded message. */
  byte[] takeMessage();	
}