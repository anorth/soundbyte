package io.soundbyte.tethered;

import io.soundbyte.core.Constants;
import io.soundbyte.core.Engine;

import java.nio.ByteBuffer;

import android.util.Log;

public class TetheredEngine implements Engine {
  
  private static final String TAG = "TetheredEngine";
  
  // Sends audio, receives messages.
  private final BufferedSocket decodingSocket;
  // Sends messages, receives audio.
  private final BufferedSocket encodingSocket;
  
  public TetheredEngine(int port, SocketListener listener) {
    decodingSocket = new BufferedSocket("decoding", port, listener);
    encodingSocket = new BufferedSocket("encoding", port + 1, listener);
  }

  @Override
  public int bytesPerSample() {
    return Constants.BYTES_PER_SAMPLE;
  }

  @Override
  public int sampleRate() {
    return Constants.SAMPLE_RATE;
  }

  @Override
  public synchronized void start() {
    Log.i(TAG, "Tethered engine starting");
    decodingSocket.start();
    encodingSocket.start();
  }
  
  @Override
  public synchronized void stop() {
    decodingSocket.close();
    encodingSocket.close();
  }
  
  @Override
  public ByteBuffer encodeMessage(byte[] payload) {
    encodingSocket.send(payload);
    byte[] received = encodingSocket.receive(10000);
    return ByteBuffer.wrap(received);
  }
    
  @Override
  public void receiveAudio(ByteBuffer audio) {
//    Log.v(TAG, "Offboarding " + audio.limit() + " bytes of audio");
    byte[] array = audio.array();
    decodingSocket.send(array);
  }

  @Override
  public boolean messageAvailable() {
    return true; // but it might be empty
  }

  @Override
  public byte[] takeMessage() {
    return decodingSocket.receive(100);
  }

  @Override
  public int messageProgress() {
    return 0;
  }
}
