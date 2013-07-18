package io.soundbyte.core;

import io.soundbyte.thirdparty.com.mixpanel.android.mpmetrics.MixpanelAPI;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.nio.ByteBuffer;
import java.util.Arrays;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.os.AsyncTask;
import android.util.Log;

public class NativeEngine implements Engine {

  private static final String TAG = "SoundbyteNativeEngine";
  private static final int MAX_MESSAGE_LENGTH = 250;
  private static final int BUFFER_DURATION_SECONDS = 10;
  private static final String LICENSE_URL_PREFIX = "http://license.soundbyte.io/key";
  private static final String MP_KEY = "f739c4a2d7db9c4c38dc0bf1a83072cc";
  
  private final String apiKey;
  private final Jni jni;
  private final MixpanelAPI mp;
  private final String installationId;
  private volatile int receiveProgress;
  private boolean started = false;

  /**
   * A configuration that generally works well: 18KHz, 8 subcarriers with 
   * spacing of two, 50 chips / second.
   */
  public static EngineConfiguration defaultConfiguration() {
    return new EngineConfiguration(18000, 8, 2, 50);
  }
  
  /**
   * Constructs an engine with default configuration.
   * 
   * @param context application context
   * @param apiKey your API key
   */
  public NativeEngine(Context context, String apiKey) {
    this(context, apiKey, defaultConfiguration());
  }
  
  /**
   * Constructs an engine.
   * 
   * @param context application context
   * @param apiKey your API key
   * @param config engine configuration
   */
  public NativeEngine(Context context, String apiKey, EngineConfiguration config) {
    if (apiKey == null || apiKey.isEmpty()) {
      throw new IllegalArgumentException("Invalid API key: " + apiKey);
    }
    this.apiKey = apiKey;
    this.jni = new Jni();
    this.installationId = Installation.id(context);
    this.mp = MixpanelAPI.getInstance(context, MP_KEY);
    jni.init(config.baseFrequency, config.subcarriers, config.subcarrierSpacing, 
        config.chipRate);
    initAnalytics(apiKey);
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
    if (!started) {
      Log.i(TAG, "Native engine starting");
      checkLicense();
      JSONObject params = new JSONObject();
      try {
        params.put("installation-id", installationId);
        params.put("new-installation", Installation.wasNewInstallation());
      } catch (JSONException e) {
        Log.e(TAG, "Soundbyte instrumentation failure", e);
      }
      mp.track("engine-started", params);
      mp.flush();
      started = true;
    }
  }

  @Override
  public synchronized void stop() {
    mp.flush();
    started = false;
  }

  @Override
  public ByteBuffer encodeMessage(byte[] payload) {
    checkStarted();
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
    checkStarted();
    receiveProgress = jni.decodeAudio(audio);
  }

  @Override
  public boolean messageAvailable() {
    checkStarted();
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

  private void checkStarted() {
    if (!started) { throw new IllegalStateException("Engine not started"); }
  }

  private ByteBuffer allocateWaveformBuffer() {
    int nbytes = sampleRate() * bytesPerSample() * BUFFER_DURATION_SECONDS;
    return ByteBuffer.allocateDirect(nbytes);
  }
  
  private void initAnalytics(String apiKey) {
    JSONObject superProperties = new JSONObject();
    try {
      superProperties.put("installation-id", installationId);
      superProperties.put("new-installation", Installation.wasNewInstallation());
      superProperties.put("api-key", apiKey);
    } catch (JSONException e) {
      Log.e(TAG, "Soundbyte instrumentation failure", e);
    }
    mp.registerSuperProperties(superProperties);
    mp.identify(apiKey);
    mp.getPeople().identify(apiKey);
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
