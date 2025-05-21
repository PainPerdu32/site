package com.example.openglesapp;

import android.app.NativeActivity;
import android.os.Bundle;
import android.view.WindowManager;

public class OpenGLActivity extends NativeActivity {

    static {
        // Load the native library.
        // The name 'opengles_app' corresponds to LOCAL_MODULE in Android.mk.
        System.loadLibrary("opengles_app");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // Keep the screen on.
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }
}
