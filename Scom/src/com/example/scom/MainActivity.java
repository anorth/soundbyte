package com.example.scom;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.widget.TextView;

import com.example.scom.Events.MessageReceived;
import com.example.scom.Events.SocketConnected;
import com.example.scom.Events.SocketDisconnected;
import com.squareup.otto.Bus;
import com.squareup.otto.Subscribe;
import com.squareup.otto.ThreadEnforcer;

public class MainActivity extends Activity {
  private static final String TAG = "AudioServerMain";
  public static final int AUDIO_PORT = 16000;


  private final Bus bus = new Bus(ThreadEnforcer.ANY);
  
  private BufferedSocket audioSocket = null;
  private BufferedSocket dataSocket = null;
  private AudioIn audioIn = null;
  private AudioOut audioOut = null;
  private DataProcessor dataProcessor = null;
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
    if (audioSocket == null) {
      audioSocket = new BufferedSocket("audio", AUDIO_PORT, bus);
      audioSocket.start();
      dataSocket = new BufferedSocket("data", AUDIO_PORT + 1, bus);
      dataSocket.start();
      audioIn = new AudioIn(audioSocket);
      audioIn.start();      
      audioOut = new AudioOut(audioSocket);
      audioOut.start();
      dataProcessor = new DataProcessor(dataSocket, bus);
      dataProcessor.start();
    }
    statusTextView.setText("Waiting...");
    getWindow().getDecorView().getRootView().setKeepScreenOn(true);
  }
  
  @Override
  public void onStop() {
    Log.w(TAG, "Received onStop");
    audioIn.close();
    audioOut.close();
    audioSocket.close();
    dataProcessor.close();
    dataSocket.close();
    try {
      audioIn.join(1000);
      audioOut.join(1000);
      audioSocket.join(1000);
      dataProcessor.join(1000);
      dataSocket.join(1000);
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
  public void socketConnected(final SocketConnected e) {
    statusTextView.post(new Runnable() {      
      @Override
      public void run() {
        statusTextView.setText("Connected " + e.name + ". Streaming...");
      }
    });
  }
  
  @Subscribe
  public void socketDisconnected(final SocketDisconnected e) {
    statusTextView.post(new Runnable() {      
      @Override
      public void run() {
        statusTextView.setText("Disconnected " + e.name + ". Waiting...");
      }
    });
  }
  
  @Subscribe
  public void messageReceived(final MessageReceived e) {
    Intent i = new Intent(Intent.ACTION_VIEW);
    i.setData(Uri.parse(e.msg));
    startActivity(i);
  }
}
