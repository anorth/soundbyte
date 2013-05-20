package io.soundbyte.app;

import io.soundbyte.app.Events.SocketConnected;
import io.soundbyte.app.Events.SocketDisconnected;
import io.soundbyte.core.Engine;
import io.soundbyte.core.NativeEngine;
import io.soundbyte.support.AudioListener;
import io.soundbyte.support.AudioPlayer;
import io.soundbyte.support.MessageReceiver;

import java.nio.charset.Charset;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.widget.TextView;

import com.squareup.otto.Bus;
import com.squareup.otto.Subscribe;
import com.squareup.otto.ThreadEnforcer;

public class MainActivity extends Activity implements MessageReceiver {
  private static final String TAG = "AudioServerMain";
  public static final int DECODER_PORT = 16000;

  private final Bus bus = new Bus(ThreadEnforcer.ANY);

  private Engine engine = null;
  private AudioListener listener = null;
  private AudioPlayer player = null;

  private boolean isForeground = false;
  private TextView statusLabel = null;  

  private String TWITTER = "com.twitter.android";

  
  private Charset UTF8 = Charset.forName("UTF-8");
  private String defaultText = "";
  private boolean sending;
  
  // TODO: fix .au
  private static final String MAPS_PREFIX = "http://m.google.com.au/u/m/";
  private static final String PLAY_PREFIX = "https://play.google.com/store/apps/details?id=";

  
  @Override
  public void onCreate(Bundle savedInstanceState) {
    Log.w(TAG, "Received onCreate");
    super.onCreate(savedInstanceState);

    Intent intent = getIntent();
    Bundle extras = intent.getExtras();
    String action = intent.getAction();
    // if this is from the share menu
    String key = Intent.EXTRA_TEXT;
    if (Intent.ACTION_SEND.equals(action)) {
      if (extras.containsKey(key)) {
        try {
          String data = extras.getString(key);
          Log.e(TAG, "Raw intent data: " + defaultText);
          String[] bits = data.split("\n");
          defaultText = bits[0]; // by default
          for (String bit : bits) {
            if (bit.startsWith(PLAY_PREFIX)) {
              String app = bit.substring(PLAY_PREFIX.length());
              if (app.equals(TWITTER)) {
                app = "twit";
              }
              defaultText = "P:" + app;
                
            } else if (bit.startsWith(MAPS_PREFIX)) {
              defaultText = "M:" + bit.substring(MAPS_PREFIX.length());
            } else if (bit.startsWith("http://")) {
              defaultText = bit;
              break;
            }
          }
        } catch (Exception e) {
          Log.e(TAG, e.toString());
        }
      } else {
        Log.e(TAG, "GOt NOTHING");
      }
    }
    setContentView(R.layout.activity_main);
    statusLabel = (TextView) findViewById(R.id.statusLabel);
    bus.register(this);
    
    statusLabel.setTextColor(Color.WHITE);
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
    player = new AudioPlayer(engine);
    player.start();
    listener = new AudioListener(engine, this, player);
    listener.start();
    getWindow().getDecorView().getRootView().setKeepScreenOn(true);
    isForeground = true;
    
    statusLabel.setText("Waiting...");    
    if (!defaultText.isEmpty()) {
      doSend();
    }
  }
  
  private void doSend() {
    sending = !sending;
    if (sending) {
      String data = defaultText;
      statusLabel.setText("Sending..."); // + data);
      Log.e(TAG, "Will send: " + data);
      engine.receiveMessage(data.getBytes(UTF8));
    } else {
      engine.receiveMessage(null);
      Log.e(TAG, "Stop sending.");
    }
  }

  @Override
  public void onStop() {
    Log.w(TAG, "Received onStop");
    listener.close();
    engine.stop();
    try {
      listener.join(1000);
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

  @Override
  public void receiveProgress(final int progress) {
    statusLabel.post(new Runnable() {
      @Override public void run() {
        if (progress == 0) {
          statusLabel.setText("Listening");
        } else {
          StringBuilder s = new StringBuilder();
          for (int i = 0; i < progress; i++) {
            s.append('-');
          }
          statusLabel.setText("Receiving " + s);
        }
      }
    });
  }

  @Override
  public void receiveMessage(byte[] messageBytes) {
    String msg = new String(messageBytes);
    if (isForeground || true) {
      String[] tokens = msg.split(":", 2);
      if (tokens.length == 2) {
         Intent i;
        if (tokens[0].equals("http")) { // for backwards compatibility
          i = new Intent(Intent.ACTION_VIEW);
          i.setData(Uri.parse(msg));
        } else if (tokens[0].equals("P")) {
          String app = tokens[1];
          if (app.equals("twit")) {
            app = TWITTER;
          }
          // Specific minified maps share url
          i = new Intent(Intent.ACTION_VIEW);
          //i.setClassName("com.google.android.apps.maps", "com.google.android.maps.MapsActivity");
          i.setData(Uri.parse(PLAY_PREFIX + app));
        } else if (tokens[0].equals("M")) {
          // Specific minified maps share url
          i = new Intent(Intent.ACTION_VIEW);
          i.setClassName("com.google.android.apps.maps", "com.google.android.maps.MapsActivity");
          i.setData(Uri.parse(MAPS_PREFIX + tokens[1]));
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
          Log.e(TAG, "Unrecognised message: " + msg);
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

  @Subscribe
  public void socketConnected(final SocketConnected e) {
    statusLabel.post(new Runnable() {
      @Override public void run() {
        statusLabel.setText("Connected " + e.name + ". Streaming...");
      }
    });
  }

  @Subscribe
  public void socketDisconnected(final SocketDisconnected e) {
    statusLabel.post(new Runnable() {
      @Override public void run() {
        statusLabel.setText("Disconnected " + e.name + ". Waiting...");
      }
    });
  }
}
