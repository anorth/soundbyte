package com.example.scom;

public class Jni {

    static {
        System.loadLibrary("scomjni");
    }
	
    public native String stringFromJNI();

}
