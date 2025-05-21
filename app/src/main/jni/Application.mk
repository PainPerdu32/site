# Application.mk
# This file specifies NDK project-wide settings.

# Specify the Android platform version.
# OpenGL ES 3.2 is available since API level 18 (Android 4.3).
# However, using a slightly higher level like 21 (Android 5.0) is common for broader NativeActivity stability.
APP_PLATFORM := android-21

# Specify the target ABIs (Application Binary Interfaces).
# For broader compatibility, include both 32-bit and 64-bit ARM.
APP_ABI := armeabi-v7a arm64-v8a

# Specify the C++ standard library to use.
# c++_shared is a common choice, but c++_static is also viable.
APP_STL := c++_shared

# Specify build mode (release or debug).
# Defaulting to debug for development.
APP_OPTIM := debug
