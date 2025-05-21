# Android.mk
# This file is used to describe your native C/C++ sources to the NDK build system.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Define the module name (must match System.loadLibrary and AndroidManifest.xml)
LOCAL_MODULE    := opengles_app

# Add C/C++ source files to compile
LOCAL_SRC_FILES := main.c egl_renderer.c

# Link against required system libraries
# -landroid: For native Android functions (like ALooper, AInputEvent)
# -lEGL: For EGL graphics library
# -lGLESv3: For OpenGL ES 3.x functions (ensure this matches your ES version)
# -llog: For Android logging (__android_log_print)
LOCAL_LDLIBS    := -landroid -lEGL -lGLESv3 -llog

# Link against the native_app_glue static library
# This library provides the NativeActivity event loop and glue code.
LOCAL_STATIC_LIBRARIES := android_native_app_glue

# Specify C compiler flags (optional)
# LOCAL_CFLAGS += -Wall -Werror

# Specify C++ compiler flags (optional, if using C++)
# LOCAL_CPPFLAGS += -Wall -Werror -std=c++17

include $(BUILD_SHARED_LIBRARY)

# Import the android_native_app_glue module from the NDK.
# This line must come *after* `include $(BUILD_SHARED_LIBRARY)`.
# Some NDK versions prefer it before, but typically it's after or at the end.
# For safety and common practice, placing it towards the end is fine.
$(call import-module,android/native_app_glue)
