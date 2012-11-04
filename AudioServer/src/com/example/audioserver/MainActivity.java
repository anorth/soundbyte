package com.example.audioserver;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;

public class MainActivity extends Activity {

  private static final String TAG = "AudioServerMain";
  private AudioSocket server = null;
  private AudioIn audioIn = null;
  private AudioOut audioOut = null;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    Log.w(TAG, "Received onCreate");
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
  }
  
  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    getMenuInflater().inflate(R.menu.activity_main, menu);
    return true;
  }

  @Override
  public void onStart() {
    super.onStart();
    Log.w(TAG, "Received onStart");
    if (server == null) {
      server = new AudioSocket();
      server.start();
      audioIn = new AudioIn(server);
      audioIn.start();      
      audioOut = new AudioOut(server);
      audioOut.start();
    }
    getWindow().getDecorView().getRootView().setKeepScreenOn(true);
  }
  
  @Override
  public void onStop() {
    Log.w(TAG, "Received onStop");
    audioIn.close();
    audioOut.close();
    server.close();
    super.onStop();
  }
  
  @Override
  public void onDestroy() {
    Log.w(TAG, "Received onDestroy");
    super.onDestroy();
  }
}
