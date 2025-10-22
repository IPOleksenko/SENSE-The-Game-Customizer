package com.ipoleksenko.sense.customizer;

import android.Manifest;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.os.StrictMode;
import android.util.Log;

import org.libsdl.app.SDLActivity;


public class MainActivity extends SDLActivity {
    private static final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder(StrictMode.getVmPolicy())
                .detectLeakedClosableObjects()
                .build());

        super.onCreate(savedInstanceState);

        if (Build.VERSION.SDK_INT >= 33) {
            requestPermissions(new String[]{Manifest.permission.READ_MEDIA_IMAGES}, 100);
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, 100);
        }

    }

    @Override
    protected String[] getLibraries() {
        return new String[]{
                "SDL2",
                "SDL2_image",
                "SDL2_ttf",
                "imgui",
                "imgui-sdl2",
                "imgui-sdlrenderer2",
                "SENSE_THE_GAME_CUSTOMIZER",
        };
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "MainActivity destroy");
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        FileManager.onActivityResult(this, requestCode, resultCode, data);
    }

}
