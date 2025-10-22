package com.ipoleksenko.sense.customizer;

import android.Manifest;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;

import androidx.annotation.NonNull;

import org.libsdl.app.SDLActivity;

public class MainActivity extends SDLActivity {
    private static final String TAG = "MainActivity";

    private static final int REQUEST_CODE_PERMISSIONS = 100;
    private AlertDialog permissionDialog;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        checkAndRequestPermissions();
    }

    private void checkAndRequestPermissions() {
        if (!hasRequiredPermissions()) {
            Log.d("MainActivity", "No permissions — requesting...");
            requestAppropriatePermissions();
        } else {
            Log.d("MainActivity", "Permissions already granted");
        }
    }

    private boolean hasRequiredPermissions() {
        if (Build.VERSION.SDK_INT >= 34) {
            return checkSelfPermission(Manifest.permission.READ_MEDIA_VISUAL_USER_SELECTED)
                    == PackageManager.PERMISSION_GRANTED;
        } else if (Build.VERSION.SDK_INT >= 33) {
            return checkSelfPermission(Manifest.permission.READ_MEDIA_IMAGES)
                    == PackageManager.PERMISSION_GRANTED;
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE)
                    == PackageManager.PERMISSION_GRANTED;
        }
        return true;
    }

    private void requestAppropriatePermissions() {
        if (Build.VERSION.SDK_INT >= 34) {
            requestPermissions(
                    new String[]{Manifest.permission.READ_MEDIA_VISUAL_USER_SELECTED},
                    REQUEST_CODE_PERMISSIONS
            );
        } else if (Build.VERSION.SDK_INT >= 33) {
            requestPermissions(
                    new String[]{Manifest.permission.READ_MEDIA_IMAGES},
                    REQUEST_CODE_PERMISSIONS
            );
        } else {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                requestPermissions(
                        new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},
                        REQUEST_CODE_PERMISSIONS
                );
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == REQUEST_CODE_PERMISSIONS) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Log.i("MainActivity", "Permission granted");
                if (permissionDialog != null && permissionDialog.isShowing()) {
                    permissionDialog.dismiss();
                }
            } else {
                boolean neverAskAgain = false;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    neverAskAgain = !shouldShowRequestPermissionRationale(permissions[0]);
                }

                if (neverAskAgain) {
                    Log.w("MainActivity", "User selected 'Don’t ask again'");
                    showGoToSettingsDialog();
                } else {
                    Log.w("MainActivity", "User denied — requesting again...");
                    requestAppropriatePermissions();
                }
            }
        }
    }

    private void showGoToSettingsDialog() {
        if (permissionDialog != null && permissionDialog.isShowing()) return;

        permissionDialog = new AlertDialog.Builder(this)
                .setTitle("Permission Required")
                .setMessage("This app needs permission to read images. Please go to settings and enable access.")
                .setCancelable(false)
                .setPositiveButton("Settings", (dialog, which) -> {
                    Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                    intent.setData(Uri.fromParts("package", getPackageName(), null));
                    startActivity(intent);
                })
                .setNegativeButton("Exit", (dialog, which) -> finishAffinity())
                .show();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (hasRequiredPermissions()) {
            if (permissionDialog != null && permissionDialog.isShowing()) {
                permissionDialog.dismiss();
            }
            Log.i("MainActivity", "Permissions confirmed after returning");
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
        Log.d(TAG, "MainActivity destroyed");
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        FileManager.onActivityResult(this, requestCode, resultCode, data);
    }
}
