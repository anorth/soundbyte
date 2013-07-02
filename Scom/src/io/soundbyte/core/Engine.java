package io.soundbyte.core;

import java.nio.ByteBuffer;

/**
 * Encodes and decodes messages to/from buffers of audio samples.
 * 
 * An engine must be thread-safe, though this may be achieved by synchronizing
 * method implementations.
 * 
 * An engine must be {@link #start() started} before encoding or decoding may occur.
 */
public interface Engine {

  /** The number of bytes representing an audio sample (usually 2). */
  int bytesPerSample();
  
  /** The audio sampling rate this engine processes (usually 44.1k Hz) */
  int sampleRate();
  
  /** 
   * Initialises engine internals. 
   * 
   * This method must be called before attempting encoding or decoding.
   */
  void start();
  
  /** Shuts down engine internals. */
  void stop();
  
  /** 
   * Encodes a complete message payload into audio data.
   * 
   * Generated samples are {{@link #bytesPerSample()} bytes per sample, little-endian, and
   * includes a preamble and envelope in addition to payload.
   * 
   * @return a buffer containing the complete encoded payload, or null if payload was empty.
   */
  public ByteBuffer encodeMessage(byte[] payload);
  
  /** 
   * Receives and processes a segment of audio data.
   * 
   * The audio should be represented as {{@link #bytesPerSample()} bytes per sample, 
   * little-endian. After this method returns,
   * {@link #messageAvailable()} indicates whether a complete message has been decoded
   * and is available. Subsequent calls are expected to present contiguous buffers
   * of audio data.
   */
  void receiveAudio(ByteBuffer audio);
  
  /** Whether a message has been decoded. */
  boolean messageAvailable();
  
  /** 
   * How much of the current message has been received, as an opaque integer.
   * 
   * If zero, then no message is currently being received. Otherwise, a positive integer
   * indicates that a message has been partially received. Progress will increase monotonically
   * until either the messages is completed ({@link #messageAvailable()} will be true) or
   * fails.
   */
  int messageProgress();
  
  /**
   * Dequeues a decoded message.
   * 
   * This is valid only if {@link #messageAvailable()} is true.
   */
  byte[] takeMessage();	
}
