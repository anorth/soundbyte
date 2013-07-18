package io.soundbyte.core;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.UUID;

import android.content.Context;
import android.util.Log;

// See http://android-developers.blogspot.com.au/2011/03/identifying-app-installations.html
public class Installation {
  private static final String INSTALLATION = "io.soundbyte.installation";

  private static String sID = null;
  private static boolean wasNewInstallation = false;

  public synchronized static String id(Context context) {
    if (sID == null) {
      File installation = new File(context.getFilesDir(), INSTALLATION);
      try {
        if (!installation.exists()) {
          wasNewInstallation = true;
          writeInstallationFile(installation);
        }
        sID = readInstallationFile(installation);
      } catch (Exception e) {
        Log.e("INSTALLATION", "Failed to read installation file", e);
        return "unknown";
      }
    }
    return sID;
  }

  // Returns whether the installation file was just created, only once.
  public static synchronized boolean wasNewInstallation() {
    try {
      return wasNewInstallation;
    } finally {
      wasNewInstallation = false;
    }
  }
  
  private static String readInstallationFile(File installation) throws IOException {
    RandomAccessFile f = new RandomAccessFile(installation, "r");
    byte[] bytes = new byte[(int) f.length()];
    f.readFully(bytes);
    f.close();
    return new String(bytes);
  }

  private static void writeInstallationFile(File installation) throws IOException {
    FileOutputStream out = new FileOutputStream(installation);
    String id = UUID.randomUUID().toString();
    out.write(id.getBytes());
    out.close();
  }
}
