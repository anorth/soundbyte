package io.soundbyte.core;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.nio.ByteBuffer;
import java.util.Arrays;

import android.os.AsyncTask;
import android.util.Log;
import android.widget.Toast;

public class NativeEngine implements Engine {

  private static final String TAG = "SoundbyteNativeEngine";
  private static final int MAX_MESSAGE_LENGTH = 250;
  private static final int BUFFER_DURATION_SECONDS = 10;
  private static final String LICENSE_URL_PREFIX = "http://license.soundbyte.io/key";
  
  private final String apiKey;
  private final Jni jni;
  private volatile int receiveProgress;

  public static EngineConfiguration defaultConfiguration() {
    return new EngineConfiguration(18000, 8, 2, 50);
  }
  
  public NativeEngine(String apiKey) {
    this(apiKey, defaultConfiguration());
  }
  
  public NativeEngine(String apiKey, EngineConfiguration config) {
    this.apiKey = apiKey;
    this.jni = new Jni();
    jni.init(config.baseFrequency, config.subcarriers, config.subcarrierSpacing, 
        config.chipRate);
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
  public void start() {
    Log.i(TAG, "Native engine starting");
    checkLicense();
  }

  @Override
  public synchronized void stop() {
  }

  @Override
  public ByteBuffer encodeMessage(byte[] payload) {
    if (payload == null) {
      return null;
    }
    
    ByteBuffer forWaveform = allocateWaveformBuffer();
    jni.encodeMessage(payload, forWaveform);
    Log.i(TAG, "Encoded " + payload.length + " byte payload into " + forWaveform.limit()
        + " bytes,  " + forWaveform.remaining() + " buffer bytes unused.");
    return forWaveform;
  }

  @Override
  public void receiveAudio(ByteBuffer audio) {
    receiveProgress = jni.decodeAudio(audio);
  }

  @Override
  public boolean messageAvailable() {
    return jni.messageAvailable();
  }

  @Override
  public byte[] takeMessage() {
    byte[] target = new byte[MAX_MESSAGE_LENGTH];
    int bytes = jni.takeMessage(target);
    if (bytes > 0) {
      return Arrays.copyOf(target, bytes);
    } else if (bytes == 0) {
      throw new RuntimeException("Message too long");
    } else {
      throw new IllegalStateException("No message available");
    }
  }
  
  @Override
  public int messageProgress() {
    return receiveProgress;
  }

  private ByteBuffer allocateWaveformBuffer() {
    int nbytes = sampleRate() * bytesPerSample() * BUFFER_DURATION_SECONDS;
    return ByteBuffer.allocateDirect(nbytes);
  }
  
  private void checkLicense() {
    AsyncTask<Void, Void, Boolean> t = new AsyncTask<Void, Void, Boolean>() {
      @Override
      protected Boolean doInBackground(Void... params) {
        try {
          URL url = new URL(LICENSE_URL_PREFIX + "/" + apiKey);
          Log.d(TAG, "Checking license");
          HttpURLConnection conn = (HttpURLConnection) url.openConnection();
          if (conn.getResponseCode() == HttpURLConnection.HTTP_OK) {
            String body = Util.slurp(conn.getInputStream()).trim();
            if (body.equals("no")) {
              Log.w(TAG, "License check indicated unlicensed use for API key " + apiKey);
              return false;
            }
            Log.d(TAG, "License check ok");
          } else {
            Log.d(TAG, "License check indeterminate - code H");
          }
        } catch (MalformedURLException e) {
          Log.e(TAG, "Failed to build license URL", e);
        } catch (SecurityException e) {
          Log.d(TAG, "License check indeterminate - code S");
        } catch (IOException e) {
          Log.d(TAG, "License check indeterminate - code C");
        }
        return true;
      }
      
      @Override
      protected void onPostExecute(Boolean result) {
        if (!result) {
          // TODO(alex): make toast, or crash, or something.
        }
      }
    };
    t.execute((Void)null);
  }
}
