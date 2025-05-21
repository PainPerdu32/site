# OpenGL ES 3.2 Android C Skeleton App

## Overview

This project provides a minimal skeleton application for Android, demonstrating the use of C, `NativeActivity`, and OpenGL ES 3.2 for rendering. It is designed to be a starting point for more complex native Android graphics applications and is compatible with devices supporting OpenGL ES 3.2, such as those based on chipsets like the MediaTek MT6739 (which typically includes a PowerVR GE8100 GPU).

The application renders a simple red triangle on a black background.

## Prerequisites

Before you begin, ensure you have the following installed and configured:

1.  **Android SDK:**
    *   Installed via Android Studio or standalone command-line tools.
    *   The `ANDROID_HOME` environment variable should be set to your SDK installation path.
2.  **Android NDK:**
    *   A reasonably recent version of the NDK is required (e.g., r21+ or the latest stable version).
    *   The NDK path should be added to your system's `PATH` environment variable, or an environment variable like `ANDROID_NDK_HOME` should be set to the NDK installation directory.

## Project Structure

The project follows a standard Android application structure with a focus on native code:

-   `app/src/main/AndroidManifest.xml`: The Android application manifest file, configured for a `NativeActivity` and specifying OpenGL ES 3.2 requirements.
-   `app/src/main/java/com/example/openglesapp/OpenGLActivity.java`: The Java `NativeActivity` class that loads the native library.
-   `app/src/main/jni/`: Contains all the native C code and NDK build scripts.
    -   `include/egl_renderer.h`: Header file for the EGL and OpenGL ES rendering logic.
    -   `main.c`: The main entry point for the native application, handling lifecycle events via `android_native_app_glue`.
    -   `egl_renderer.c`: Implementation of EGL setup, OpenGL ES 3.2 rendering (a simple triangle), and event handling.
    -   `Android.mk`: Build script for `ndk-build`, defining the native module, source files, and linked libraries.
    -   `Application.mk`: Specifies NDK project-wide settings like target ABIs (`armeabi-v7a`, `arm64-v8a`) and the Android platform version.
-   `app/src/main/libs/`: This directory will be created by `ndk-build` and will contain the compiled native shared libraries (`.so` files) for each ABI.
-   `app/src/main/jniLibs/`: (Often used by Gradle) If using Android Studio, this is a common location for pre-compiled native libraries.
-   `app/src/main/res/`: Contains Android resources (though minimal in this skeleton).

## Building the Application

There are two main ways to build this application: using `ndk-build` directly for the native components, and then using Android Studio (or Gradle) to package the final APK.

**1. Compile Native Code with `ndk-build`:**

   a. Open a terminal or command prompt.
   b. Navigate to the JNI source directory:
      ```bash
      cd path/to/your/project/OpenGLSkeleton/app/src/main/jni
      ```
   c. Run the `ndk-build` command:
      ```bash
      ndk-build
      ```
      This command compiles the C code using the `Android.mk` and `Application.mk` configurations. Upon successful compilation, it will generate shared object (`.so`) files and place them into `app/src/main/libs/<ABI>/libopengles_app.so` (e.g., `app/src/main/libs/armeabi-v7a/libopengles_app.so`).

**2. Package the APK using Android Studio (Recommended):**

   a. **Import Project:**
      - Open Android Studio.
      - Choose "Open an existing Android Studio project" (or "File" > "Open...") and select the root directory of this project (`OpenGLSkeleton`).
   b. **Configure Native Libraries:**
      - Android Studio's Gradle build system needs to know where to find the native libraries compiled by `ndk-build`.
      - By default, `ndk-build` places output in `app/src/main/libs`. Gradle typically looks in `app/src/main/jniLibs`.
      - **Option 1 (Simple Copy):** After running `ndk-build` as described above, copy the entire `app/src/main/libs` directory and rename it to `app/src/main/jniLibs` within the `app/src/main/` directory.
        ```
        app/src/main/
             ├── java/
             ├── jni/
             ├── libs/  <-- Output of ndk-build
             └── jniLibs/ <-- Copy of libs, for Gradle
        ```
      - **Option 2 (Gradle Configuration):** Alternatively, you can tell Gradle to look in the `libs` directory by modifying `app/build.gradle` (Module :app) and adding/modifying the `sourceSets` block within the `android` block:
        ```gradle
        android {
            // ... other configurations ...
            sourceSets {
                main {
                    jniLibs.srcDirs = ['src/main/libs']
                }
            }
        }
        ```
        If you choose this option, ensure the `app/build.gradle` file is updated accordingly. For this skeleton, we assume the simpler copy method or that the default `jniLibs` path is used.
   c. **Build APK:**
      - Once Android Studio has synced the project, build the APK by selecting "Build" > "Build Bundle(s) / APK(s)" > "Build APK(s)".
      - The generated APK (e.g., `app-debug.apk`) will be located in `app/build/outputs/apk/debug/`.

## Running the Application

1.  **Ensure Device is Connected:** Connect an Android device that supports OpenGL ES 3.2 to your computer via USB, with USB debugging enabled.
2.  **Install APK:**
    *   **Using Android Studio:** Click the "Run" button (green play icon) in Android Studio, selecting your connected device.
    *   **Using ADB:** If you built the APK manually, you can install it using the Android Debug Bridge (ADB):
        ```bash
        adb install path/to/your/OpenGLSkeleton/app/build/outputs/apk/debug/app-debug.apk
        ```
3.  **Launch:**
    *   The application should install and launch automatically if run from Android Studio. Otherwise, find "OpenGL ES App" in your device's app drawer and open it.
    *   Upon launching, the application should display a **red triangle on a black background**.

## Troubleshooting

-   **`ndk-build: command not found`**:
    Ensure your NDK directory is correctly added to your system's `PATH` environment variable, or use the full path to `ndk-build`.
-   **`ANDROID_HOME` not set**:
    Ensure the `ANDROID_HOME` environment variable is set to your Android SDK installation path. Some tools might also rely on `ANDROID_SDK_ROOT`.
-   **ABI Compatibility Issues**:
    The application is configured to build for `armeabi-v7a` and `arm64-v8a`. Ensure your target device supports one of these ABIs. Most modern devices support `arm64-v8a`. If you encounter an `INSTALL_FAILED_NO_MATCHING_ABIS` error during installation, it means the device's architecture is not compatible with the ABIs included in your APK.
-   **OpenGL ES Version Errors / EGL Initialization Errors**:
    -   If the app crashes on startup or shows a black screen without the triangle, check `logcat` (e.g., `adb logcat | grep "OpenGLApp"`) for errors related to EGL initialization (`eglGetDisplay`, `eglChooseConfig`, `eglCreateContext`, etc.) or OpenGL ES calls.
    -   Ensure your device's GPU drivers correctly support OpenGL ES 3.2. The manifest requests ES 3.2, and the context creation in `egl_renderer.c` specifically asks for an ES3 client version.
-   **Shader Compilation/Linking Errors**:
    Check `logcat` for errors reported during shader compilation or program linking in `egl_renderer.c`. The error messages are logged to help diagnose GLSL syntax issues or other shader-related problems.
-   **Build Issues in Android Studio**:
    If Android Studio has trouble finding native symbols or complains about NDK configuration:
    - Ensure the NDK path is correctly configured in Android Studio ("File" > "Project Structure" > "SDK Location" > "Android NDK location").
    - Clean the project ("Build" > "Clean Project") and rebuild.
    - Verify that the `jniLibs` source directory is correctly set up in `app/build.gradle` if you are not manually copying the `.so` files.

This README should provide a good starting point for users to build and run the skeleton application.The `README.md` file has been updated with the comprehensive information as requested.
It includes:
- Title: "OpenGL ES 3.2 Android C Skeleton App"
- Overview: Description of the app, its purpose, and target compatibility.
- Prerequisites: Android SDK and NDK setup instructions.
- Project Structure: List of key directories and files.
- Building the Application: Detailed steps for both `ndk-build` and Android Studio, including native library path configuration for Gradle.
- Running the Application: Instructions for installing and launching the APK, and the expected output.
- Troubleshooting: Common issues related to NDK paths, ABI compatibility, and OpenGL ES errors.

This should provide users with sufficient guidance to get the project up and running.
