package com.example.audioserver;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;

public class MainActivity extends Activity {

  private static final String TAG = "AudioServerMain";
  private AudioServer server = null;
  private AudioIn audio = null;
  
  @Override
  public void onCreate(Bundle savedInstanceState) {
    Log.w(TAG, "Received onCreate");
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    server = new AudioServer();
    server.start();
    audio = new AudioIn(server);
    audio.start();
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    getMenuInflater().inflate(R.menu.activity_main, menu);
    return true;
  }
  
  @Override
  public void onStop() {
    Log.w(TAG, "Received onStop");
    audio.close();
    server.close();
    super.onStop();
  }
}
