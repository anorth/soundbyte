package io.soundbyte.soundbytedemo;

import io.soundbyte.core.Engine;
import io.soundbyte.core.NativeEngine;
import io.soundbyte.support.AudioListener;
import io.soundbyte.support.AudioPlayer;
import io.soundbyte.support.MessageReceiver;

import java.nio.charset.Charset;

import android.app.Activity;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class MainActivity extends Activity implements MessageReceiver {
  private static final String TAG = "MainActivity";
  private static final Charset UTF8 = Charset.forName("UTF-8");

  private Engine engine = null;
  private AudioListener listener = null;
  private AudioPlayer player = null;
  
  private EditText sendInput;
  private Button sendButton;
  private TextView progressText;
  private TextView messagesText;
  private ScrollingMovementMethod messagesScroll = new ScrollingMovementMethod();
  private boolean isSending = false;
    
  @Override
  public void onCreate(Bundle savedInstanceState) {
    Log.d(TAG, "Received onCreate");
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    
    sendInput = (EditText) findViewById(R.id.sendInput);
    sendButton = (Button) findViewById(R.id.sendButton);
    progressText = (TextView) findViewById(R.id.progressText);
    messagesText = (TextView) findViewById(R.id.receivedMessagesText);
    messagesText.setMovementMethod(messagesScroll);
    
    sendButton.setOnClickListener(new OnClickListener() {      
      @Override
      public void onClick(View v) {
        doSend();
      }
    });
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
    String msg = sendInput.getText().toString();
    if (!msg.isEmpty()) {
      isSending = !isSending;
    }
    if (isSending) {
      String data = sendInput.getText().toString();
      Log.i(TAG, "Will send: " + data);
      player.sendMessage(data.getBytes(UTF8));      
      sendButton.setText("Stop");
    } else {
      Log.i(TAG, "Stopping");
      player.sendMessage(null);
      sendButton.setText("Send");
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
    progressText.post(new Runnable() {
      @Override public void run() {
        if (progress == 0) {
          progressText.setText("¥");
        } else {
          StringBuilder s = new StringBuilder();
          for (int i = 0; i < progress; i++) {
            s.append('-');
          }
          progressText.setText("* " + s);
        }
      }
    });
  }

  @Override
  public void receiveMessage(byte[] messageBytes) {
    final String msg = new String(messageBytes);
    Log.i(TAG, "Received: " + msg);
    messagesText.post(new Runnable() {
      @Override
      public void run() {
        messagesText.setText(messagesText.getText() + msg + "\n");
      }
    });
  }
}
