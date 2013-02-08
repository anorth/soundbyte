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
  
  /**
   * Encodes a message payload into a chunk of audio encapsulating the complete message.
   */
  ByteBuffer encode(byte[] payload);

  /**
   * Receives and synchronously processes segment of audio data.
   * 
   * After this method returns, {@link #messageAvailable()} indicates whether one or more 
   * complete messages have been decoded.
   */
  void receiveAudio(ByteBuffer audio);
  
  /**
   * Whether a message has been decoded.
   */
  boolean messageAvailable();
  
  /**
   * Dequeues a decoded message.
   */
  byte[] takeMessage();	
}
