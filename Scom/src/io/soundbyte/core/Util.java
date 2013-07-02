package io.soundbyte.core;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.UnsupportedEncodingException;

public class Util {
  
  /** Reads an input stream to completion */
  public static String slurp(InputStream is) throws IOException {
    char[] buffer = new char[100];
    StringBuilder out = new StringBuilder();
    try {
      Reader in = new InputStreamReader(is, "UTF-8");
      try {
        for (;;) {
          int rsz = in.read(buffer, 0, buffer.length);
          if (rsz < 0)
            break;
          out.append(buffer, 0, rsz);
        }
      } finally {
        in.close();
      }
    } catch (UnsupportedEncodingException ex) {
      throw new RuntimeException(ex);
    }
    return out.toString();
  }
  
  private Util() {}
}
