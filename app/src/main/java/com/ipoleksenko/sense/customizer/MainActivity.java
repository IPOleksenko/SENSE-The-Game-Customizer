package com.ipoleksenko.sense.customizer;

import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.os.StrictMode;

import org.libsdl.app.SDLActivity;

public class MainActivity extends SDLActivity
{
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
    protected void onCreate(Bundle savedInstanceState) {
        StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder(StrictMode.getVmPolicy())
            .detectLeakedClosableObjects()
            .build());

        if (android.os.Build.VERSION.SDK_INT >= 9) {
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        } else {
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT);
        }

        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onDestroy() {
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
        super.onDestroy();
    }
}
