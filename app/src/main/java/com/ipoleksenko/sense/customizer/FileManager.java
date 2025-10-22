package com.ipoleksenko.sense.customizer;

import android.Manifest;
import android.app.Activity;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.StrictMode;
import android.provider.MediaStore;
import android.util.Log;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.documentfile.provider.DocumentFile;

import org.libsdl.app.SDLActivity;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.nio.file.Files;
import java.util.Collections;
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

    public static native void nativeOnImagePicked(String imagePath, byte[] imageData);

    public static void pickImageFromGallery() {
        Activity activity = (Activity) SDLActivity.getContext();
        if (activity == null) {
            Log.e(TAG, "SDLActivity.getContext() returned null — cannot open file picker");
            return;
        }

        activity.runOnUiThread(() -> {
            try {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.setType("image/png");
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.putExtra(Intent.EXTRA_LOCAL_ONLY, true);
                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);

                activity.startActivityForResult(intent, PICK_IMAGE_REQUEST);
                Log.i(TAG, "Local file picker started (no cloud options)");
            } catch (Exception e) {
                Log.e(TAG, "Failed to start local file picker", e);
            }
        });
    }

    public static void onActivityResult(Activity activity, int requestCode, int resultCode, Intent data) {
        if (requestCode == PICK_IMAGE_REQUEST && resultCode == Activity.RESULT_OK && data != null) {
            Uri uri = data.getData();
            if (uri == null) return;

            try (InputStream inputStream = activity.getContentResolver().openInputStream(uri)) {
                if (inputStream == null) {
                    Log.e(TAG, "Cannot open input stream for: " + uri);
                    return;
                }

                String fileName = getFileName(activity, uri);
                if (fileName == null) fileName = "unknown.png";

                File decorDir = new File(activity.getFilesDir(), "decor");
                if (!decorDir.exists()) decorDir.mkdirs();

                File outFile = new File(decorDir, fileName);

                Log.i(TAG, "Copying content URI to local file: " + outFile.getAbsolutePath());

                try (OutputStream out = new FileOutputStream(outFile)) {
                    byte[] buffer = new byte[8192];
                    int bytesRead;
                    while ((bytesRead = inputStream.read(buffer)) != -1) {
                        out.write(buffer, 0, bytesRead);
                    }
                    out.flush();
                }

                byte[] bytes = null;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    bytes = Files.readAllBytes(outFile.toPath());
                }

                Log.i(TAG, "Local file ready: " + outFile.getAbsolutePath() + " (" + bytes.length + " bytes)");

                nativeOnImagePicked(outFile.getAbsolutePath(), bytes);

            } catch (Exception e) {
                Log.e(TAG, "Error processing image URI: " + data.getData(), e);
            }
        }
    }

    private static String getFileName(Context context, Uri uri) {
        String result = null;
        Cursor cursor = null;
        try {
            cursor = context.getContentResolver().query(uri, null, null, null, null);
            if (cursor != null && cursor.moveToFirst()) {
                int nameIndex = cursor.getColumnIndex(MediaStore.Images.Media.DISPLAY_NAME);
                if (nameIndex >= 0)
                    result = cursor.getString(nameIndex);
            }
        } catch (Exception e) {
            Log.w(TAG, "Cannot resolve file name for: " + uri, e);
        } finally {
            if (cursor != null) cursor.close();
        }
        return result;
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
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(context.getContentResolver().openInputStream(uri))
        )) {
            String line;
            while ((line = reader.readLine()) != null) {
                textBuilder.append(line).append('\n');
            }
        } catch (IOException e) {
            Log.e(TAG, "Error reading file from URI: " + uri, e);
        } catch (Exception e) {
            Log.e(TAG, "Unexpected error while reading file: " + uri, e);
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
        try (OutputStream outputStream = context.getContentResolver().openOutputStream(uri, "rwt");
             BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(outputStream))) {

            if (outputStream == null) {
                Log.w(TAG, "Failed to open output stream for writing: " + uri);
                return false;
            }

            for (String line : lines) {
                writer.write(line);
                writer.newLine();
            }
            writer.flush();

            Log.i(TAG, "File successfully written: " + uri);
            return true;
        } catch (IOException e) {
            Log.e(TAG, "Error while writing file " + uri, e);
            return false;
        } catch (Exception e) {
            Log.e(TAG, "Unexpected error while writing file " + uri, e);
            return false;
        }
    }

    public static boolean copyFile(Context context, String sourcePath, String targetName) {
        InputStream inputStream = null;
        OutputStream outputStream = null;

        try {
            File srcFile = new File(sourcePath);
            Log.i(TAG, "Checking source file: " + srcFile.getAbsolutePath());
            Log.i(TAG, "File length (exists check) = " + srcFile.length());

            if (!srcFile.exists()) {
                Log.e(TAG, "Source file does NOT exist!");
                return false;
            }

            if (!srcFile.canRead()) {
                Log.e(TAG, "Source file is NOT readable!");
            } else {
                Log.i(TAG, "Source file is readable.");
            }

            try {
                inputStream = new FileInputStream(srcFile);
            } catch (FileNotFoundException fnf) {
                Log.e(TAG, "FileInputStream FAILED", fnf);
                return false;
            }

            int available = inputStream.available();
            Log.i(TAG, "InputStream.available() = " + available);

            String targetPath = "decor/" + targetName;
            Log.i(TAG, "Calling createFile(context, " + targetPath + ")");
            createFile(context, targetPath);

            Uri destUri = Uri.parse(PROVIDER_URI + targetPath);
            Log.i(TAG, "Destination URI: " + destUri);

            try {
                outputStream = context.getContentResolver().openOutputStream(destUri, "w");
            } catch (FileNotFoundException fnf) {
                Log.e(TAG, "openOutputStream FAILED", fnf);
                return false;
            }

            if (outputStream == null) {
                Log.e(TAG, "Output stream is NULL for: " + destUri);
                return false;
            }

            Log.i(TAG, "Output stream opened successfully.");

            byte[] buffer = new byte[8192];
            int bytesRead;
            int totalBytes = 0;
            long start = System.currentTimeMillis();

            Log.i(TAG, "Starting byte copy...");
            while ((bytesRead = inputStream.read(buffer)) != -1) {
                outputStream.write(buffer, 0, bytesRead);
                totalBytes += bytesRead;
                if (totalBytes % 65536 < 8192) {
                    Log.i(TAG, "Copied " + totalBytes + " bytes so far...");
                }
            }

            outputStream.flush();
            long elapsed = System.currentTimeMillis() - start;

            Log.i(TAG, "Copy complete: " + totalBytes + " bytes in " + elapsed + " ms");
            Log.i(TAG, "File saved to: " + destUri);

             DocumentFile decorDir = DocumentFile.fromTreeUri(context, Uri.parse(PROVIDER_URI + "decor"));
            if (decorDir != null) {
                DocumentFile[] files = decorDir.listFiles();
                for (DocumentFile f : files) {
                    if (f.getName().equals(targetName)) {
                        Log.i(TAG, "Result file found: " + f.getUri() + " (" + f.length() + " bytes)");
                    }
                }
            }

            return true;

        } catch (Exception e) {
            Log.e(TAG, "Exception while copying file!", e);
            return false;

        } finally {
            try {
                if (inputStream != null) {
                    inputStream.close();
                    Log.i(TAG, "Input stream closed.");
                }
                if (outputStream != null) {
                    outputStream.close();
                    Log.i(TAG, "Output stream closed.");
                }
            } catch (IOException ex) {
                Log.e(TAG, "Error closing streams", ex);
            }
        }
    }
    public static boolean deleteDecorFile(Context context, String filePath) {
        if (filePath == null || filePath.isEmpty()) {
            Log.e(TAG, "deleteDecorFile: path is null or empty");
            return false;
        }

        try {
            File file = new File(filePath);
            String fileName = file.getName();
            Log.i(TAG, "Extracted file name: " + fileName);

            String targetPath = "decor/" + fileName;
            Uri uri = Uri.parse(PROVIDER_URI + targetPath);
            Log.i(TAG, "Target URI for delete: " + uri);

            int rowsDeleted = context.getContentResolver().delete(uri, null, null);

            if (rowsDeleted > 0) {
                Log.i(TAG, "File deleted successfully: " + fileName);
                return true;
            } else {
                Log.w(TAG, "File not found or failed to delete: " + fileName);
                return false;
            }

        } catch (Exception e) {
            Log.e(TAG, "Exception while deleting decor file: " + filePath, e);
            return false;
        }
    }
    public static boolean renameDecorFile(Context context, String filePath, String newName) {
        if (filePath == null || filePath.isEmpty() || newName == null || newName.isEmpty()) {
            Log.e(TAG, "renameDecorFile: invalid arguments");
            return false;
        }

        try {
            File file = new File(filePath);
            String oldName = file.getName();
            Log.i(TAG, "Old name: " + oldName + " → New name: " + newName);

            String targetPath = "decor/" + oldName;
            Uri uri = Uri.parse(PROVIDER_URI + targetPath);
            Log.i(TAG, "Target URI for rename: " + uri);

            ContentValues values = new ContentValues();
            values.put("new_name", newName);

            int rowsUpdated = context.getContentResolver().update(uri, values, null, null);

            if (rowsUpdated > 0) {
                Log.i(TAG, "File renamed successfully: " + oldName + " → " + newName);
                return true;
            } else {
                Log.w(TAG, "Failed to rename file via ContentProvider: " + oldName);
                return false;
            }

        } catch (Exception e) {
            Log.e(TAG, "Exception while renaming decor file: " + filePath, e);
            return false;
        }
    }

}
