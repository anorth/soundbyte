package com.example.audioserver;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.widget.TextView;

import com.example.audioserver.Events.SocketConnected;
import com.example.audioserver.Events.SocketDisconnected;
import com.squareup.otto.Bus;
import com.squareup.otto.Subscribe;
import com.squareup.otto.ThreadEnforcer;

public class MainActivity extends Activity {
  private static final String TAG = "AudioServerMain";

  private final Bus bus = new Bus(ThreadEnforcer.ANY);
  
  private AudioSocket server = null;
  private AudioIn audioIn = null;
  private AudioOut audioOut = null;
  private TextView statusTextView = null;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    Log.w(TAG, "Received onCreate");
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    statusTextView = (TextView) findViewById(R.id.textView4);
    bus.register(this);
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
      server = new AudioSocket(bus);
      server.start();
      audioIn = new AudioIn(server);
      audioIn.start();      
      audioOut = new AudioOut(server);
      audioOut.start();
    }
    statusTextView.setText("Waiting...");
    getWindow().getDecorView().getRootView().setKeepScreenOn(true);
  }
  
  @Override
  public void onStop() {
    Log.w(TAG, "Received onStop");
    audioIn.close();
    audioOut.close();
    server.close();
    try {
      audioIn.join(1000);
      audioOut.join(1000);
      server.join(1000);
    } catch (InterruptedException e) {
      Log.w(TAG, "Interrupted waiting for threads to close");
    }
    super.onStop();
  }
  
  @Override
  public void onDestroy() {
    Log.w(TAG, "Received onDestroy");
    super.onDestroy();
  }
  
  @Subscribe
  public void socketConnected(SocketConnected e) {
    statusTextView.post(new Runnable() {      
      @Override
      public void run() {
        statusTextView.setText("Connected. Streaming audio...");
      }
    });
  }
  
  @Subscribe
  public void socketDisconnected(SocketDisconnected e) {
    statusTextView.post(new Runnable() {      
      @Override
      public void run() {
        statusTextView.setText("Disconnected. Waiting...");
      }
    });
  }
}
