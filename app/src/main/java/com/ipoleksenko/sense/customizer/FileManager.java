package com.ipoleksenko.sense.customizer;

import android.Manifest;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.StrictMode;
import android.util.Log;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import org.libsdl.app.SDLActivity;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.util.List;

public class FileManager extends SDLActivity {
    private static final String TAG = "JFileManager";
    private static final int REQUEST_PERMISSION_CODE = 0;
    private static final int PICK_IMAGE_REQUEST = 1;
    private static final String PROVIDER_URI = "content://com.ipoleksenko.sense.provider/";

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

    private void pickImageFromGallery() {
        String permission;

        if (Build.VERSION.SDK_INT >= 33) {
            permission = Manifest.permission.READ_MEDIA_IMAGES;
        } else {
            permission = Manifest.permission.READ_EXTERNAL_STORAGE;
        }

        if (ContextCompat.checkSelfPermission(this, permission)
                != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{permission}, REQUEST_PERMISSION_CODE);
        } else {
            openGallery();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_PERMISSION_CODE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                openGallery();
            }
        }
    }

    private void openGallery() {
        Intent pickIntent = new Intent(Intent.ACTION_PICK);
        pickIntent.setType("image/png");
        startActivityForResult(pickIntent, PICK_IMAGE_REQUEST);
    }

    public static String readFullText(Context context, String path) {
        Log.w(TAG, "readFullText called with path: " + path);
        StringBuilder textBuilder = new StringBuilder();

        Uri uri = Uri.parse(PROVIDER_URI + path);
        Log.w(TAG, "readFullText uri path: " + uri);
        BufferedReader reader = null;

        try {
            reader = new BufferedReader(
                    new InputStreamReader(context.getContentResolver().openInputStream(uri))
            );

            String line;
            while ((line = reader.readLine()) != null) {
                textBuilder.append(line).append('\n');
            }
        } catch (IOException e) {
            Log.e(TAG, "Error reading file from URI: " + uri, e);
        } catch (Exception e) {
            Log.e(TAG, "Unexpected error while reading file: " + uri, e);
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException e) {
                    Log.w(TAG, "Error closing reader: " + e.getMessage());
                }
            }
        }

        return textBuilder.toString();
    }

    public static boolean createFile(Context context, String fileName) {
        try {
            Uri uri = Uri.parse(PROVIDER_URI + fileName);
            ContentValues values = new ContentValues();
            values.put("name", fileName);

            Uri result = context.getContentResolver().insert(uri, values);
            boolean success = result != null;
            Log.i(TAG, success
                    ? "File created through ContentProvider: " + result
                    : "Failed to create file through ContentProvider: " + fileName);
            return success;
        } catch (Exception e) {
            Log.e(TAG, "Error while creating file: " + fileName, e);
            return false;
        }
    }

    public static boolean fdExists(Context context, String path) {
        Uri uri = Uri.parse(PROVIDER_URI + path);
        Cursor cursor = null;
        boolean exists = false;

        try {
            cursor = context.getContentResolver().query(uri, null, null, null, null);

            if (cursor != null && cursor.moveToFirst()) {
                int nameIndex = cursor.getColumnIndex("name");
                if (nameIndex != -1) {
                    do {
                        String currentName = cursor.getString(nameIndex);
                        if (path.equals(currentName)) {
                            exists = true;
                            break;
                        }
                    } while (cursor.moveToNext());
                } else {
                    exists = true;
                }
            }

        } catch (Exception e) {
            Log.e(TAG, "Error while checking file existence: " + path, e);
        } finally {
            if (cursor != null) cursor.close();
        }

        Log.i(TAG, "File check \"" + path + "\": " + exists);
        return exists;
    }

    public static boolean writeTextFile(Context context, String path, List<String> lines) {
        Uri uri = Uri.parse(PROVIDER_URI + path);
        OutputStream outputStream = null;

        try {
            outputStream = context.getContentResolver().openOutputStream(uri, "rwt");
            if (outputStream == null) {
                Log.w(TAG, "Failed to open output stream for writing: " + uri);
                return false;
            }

            BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(outputStream));
            for (String line : lines) {
                writer.write(line);
                writer.newLine();
            }
            writer.flush();
            writer.close();

            Log.i(TAG, "File successfully written: " + uri);
            return true;
        } catch (IOException e) {
            Log.e(TAG, "Error while writing file " + uri, e);
            return false;
        } catch (Exception e) {
            Log.e(TAG, "Unexpected error while writing file " + uri, e);
            return false;
        } finally {
            if (outputStream != null) {
                try {
                    outputStream.close();
                } catch (IOException e) {
                    Log.w(TAG, "Error while closing stream: " + e.getMessage());
                }
            }
        }
    }
}