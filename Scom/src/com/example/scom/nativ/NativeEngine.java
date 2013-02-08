package com.example.scom.nativ;

import java.nio.ByteBuffer;

import android.util.Log;

import com.example.scom.Engine;

public class NativeEngine implements Engine {

  private static final String TAG = "NativeEngine";
  
  private final Jni jni;
  
  public NativeEngine() {
    jni = new Jni();
  }
  
  @Override
  public void start() {
    Log.i(TAG, "Native engine starting: " + jni.stringFromJNI());
  }

  @Override
  public void stop() {
    // TODO Auto-generated method stub

  }

  @Override
  public void receiveMessage(byte[] payload) {
    // TODO Auto-generated method stub

  }

  @Override
  public boolean audioAvailable() {
    // TODO Auto-generated method stub
    return false;
  }

  @Override
  public ByteBuffer takeAudio() {
    // TODO Auto-generated method stub
    return null;
  }

  @Override
  public void receiveAudio(ByteBuffer audio) {
    // TODO Auto-generated method stub

  }

  @Override
  public boolean messageAvailable() {
    // TODO Auto-generated method stub
    return false;
  }

  @Override
  public byte[] takeMessage() {
    // TODO Auto-generated method stub
    return null;
  }

}
