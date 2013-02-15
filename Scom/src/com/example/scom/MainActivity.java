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
import com.example.scom.nativ.NativeEngine;
import com.example.scom.tethered.TetheredEngine;
import com.squareup.otto.Bus;
import com.squareup.otto.Subscribe;
import com.squareup.otto.ThreadEnforcer;

public class MainActivity extends Activity {
  private static final String TAG = "AudioServerMain";
  public static final int DECODER_PORT = 16000;

  private final Bus bus = new Bus(ThreadEnforcer.ANY);

  private Engine engine = null;
  private AudioListener audioIn = null;
  private AudioPlayer audioOut = null;
  private DataProcessor dataProcessor = null;

  private boolean isForeground = false;
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
    engine = new NativeEngine();
//    engine = new TetheredEngine(DECODER_PORT, bus);
    engine.start();
    audioIn = new AudioListener(engine);
    audioIn.start();
    audioOut = new AudioPlayer(engine);
    audioOut.start();
    dataProcessor = new DataProcessor(engine, bus);
    dataProcessor.start();
    statusTextView.setText("Waiting...");
    getWindow().getDecorView().getRootView().setKeepScreenOn(true);
    isForeground = true;
  }

  @Override
  public void onStop() {
    Log.w(TAG, "Received onStop");
    audioIn.close();
    engine.stop();
    dataProcessor.close();
    try {
      audioIn.join(1000);
      dataProcessor.join(1000);
    } catch (InterruptedException e) {
      Log.e(TAG, "Interrupted waiting for threads to close");
    }
    isForeground = false;
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
    if (isForeground) {
      String[] tokens = e.msg.split(":", 2);
      if (tokens.length == 2) {
         Intent i;
        if (tokens[0].equals("http")) { // for backwards compatibility
          i = new Intent(Intent.ACTION_VIEW);
          i.setData(Uri.parse(e.msg));
        } else if (tokens[0].equals("u")) {
          i = new Intent(Intent.ACTION_VIEW);
          i.setData(Uri.parse(tokens[1]));
        } else if (tokens[0].equals("m")) {
          i = new Intent(Intent.ACTION_VIEW);
          // alt "google.navigation:q=an+address+city"
          i.setClassName("com.google.android.apps.maps", "com.google.android.maps.MapsActivity");
          i.setData(Uri.parse("http://maps.google.com/maps?" + tokens[1]));
        } else if (tokens[0].equals("n")) {
          i = new Intent(Intent.ACTION_VIEW);
          i.setData(Uri.parse("google.navigation:" + tokens[1]));
        } else {
          Log.e(TAG, "Unrecognised message: " + e.msg);
          return;
        }
        try {
          startActivity(i);        
        } catch (RuntimeException ex) {
          Log.w(TAG, String.format("Failed to launch activity with intent %s", i), ex);
        }
      }
      
    }
  }
}
