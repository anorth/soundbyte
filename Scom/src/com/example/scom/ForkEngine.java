// Copyright 2013 Helix Technologies Australia.

package com.example.scom;

import java.nio.ByteBuffer;

/**
 *
 * @author dan@helixta.com.au
 */
public class ForkEngine implements Engine {
  private final Engine main;
  private final Engine secondary;

  /**
   * @param main
   * @param secondary
   */
  public ForkEngine(Engine main, Engine secondary) {
    super();
    this.main = main;
    this.secondary = secondary;
  }

  @Override
  public void start() {
    main.start();
    secondary.start();
  }

  @Override
  public void stop() {
    main.stop();
    secondary.stop();
  }

  @Override
  public void receiveMessage(byte[] payload) {
    main.receiveMessage(payload);
  }

  @Override
  public boolean audioAvailable() {
    return main.audioAvailable();
  }

  @Override
  public ByteBuffer takeAudio() {
    return main.takeAudio();
  }

  @Override
  public void receiveAudio(ByteBuffer audio) {
    main.receiveAudio(audio);
    secondary.receiveAudio(audio);
  }

  @Override
  public boolean messageAvailable() {
    return main.messageAvailable();
  }

  @Override
  public byte[] takeMessage() {
    return main.takeMessage();
  }

}
