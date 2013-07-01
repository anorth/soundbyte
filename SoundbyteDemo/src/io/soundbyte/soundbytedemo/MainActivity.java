package io.soundbyte.soundbytedemo;

import io.soundbyte.core.Engine;
import io.soundbyte.core.NativeEngine;
import io.soundbyte.support.AudioListener;
import io.soundbyte.support.AudioPlayer;
import io.soundbyte.support.MessageReceiver;

import java.nio.charset.Charset;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;

public class MainActivity extends Activity implements MessageReceiver {
  private static final String TAG = "MainActivity";

  private Engine engine = null;
  private AudioListener listener = null;
  private AudioPlayer player = null;
  
  private Charset UTF8 = Charset.forName("UTF-8");
  private boolean sending;
    
  @Override
  public void onCreate(Bundle savedInstanceState) {
    Log.d(TAG, "Received onCreate");
    super.onCreate(savedInstanceState);

    setContentView(R.layout.activity_main);
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    getMenuInflater().inflate(R.menu.main, menu);
    return true;
  }

  @Override
  public void onStart() {
    super.onStart();
    Log.d(TAG, "Received onStart");
    engine = new NativeEngine();
    engine.start();
    player = new AudioPlayer(engine);
    player.start();
    listener = new AudioListener(engine, this, player);
    listener.start();
  }
  
  private void doSend() {
    sending = !sending;
    if (sending) {
      String data = "FIXME";
      Log.i(TAG, "Will send: " + data);
      player.sendMessage(data.getBytes(UTF8));
    }
  }

  @Override
  public void onStop() {
    Log.d(TAG, "Received onStop");
    listener.close();
    player.close();
    engine.stop();
    try {
      listener.join(1000);
      player.join(5000);
    } catch (InterruptedException e) {
      Log.e(TAG, "Interrupted waiting for threads to close");
    }
    super.onStop();
  }

  @Override
  public void onDestroy() {
    Log.w(TAG, "Received onDestroy");
    super.onDestroy();
  }

  @Override
  public void receiveProgress(final int progress) {
//    statusLabel.post(new Runnable() {
//      @Override public void run() {
//        if (progress == 0) {
//          statusLabel.setText("Listening");
//        } else {
//          StringBuilder s = new StringBuilder();
//          for (int i = 0; i < progress; i++) {
//            s.append('-');
//          }
//          statusLabel.setText("Receiving " + s);
//        }
//      }
//    });
  }

  @Override
  public void receiveMessage(byte[] messageBytes) {
    String msg = new String(messageBytes);
    Log.i(TAG, "Received: " + msg);
  }
}
