package com.example.scom.nativ;

public class Jni {

    static {
        System.loadLibrary("scomjni");
    }
	
    public native String stringFromJNI();

}
