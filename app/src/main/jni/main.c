// main.c
// Main entry point for the native Android application.
// Handles application lifecycle events and delegates rendering to egl_renderer.

#include <android_native_app_glue.h>
#include <android/log.h>
#include <stdlib.h> // For malloc and free
#include "include/egl_renderer.h"

#define LOG_TAG "OpenGLApp_Main"
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

void android_main(struct android_app* app) {
    // Make sure glue isn't stripped.
    app_dummy();

    // Allocate RendererState. It's important to manage its lifecycle.
    // It will be freed when APP_CMD_DESTROY is received, or before exiting if destroyRequested.
    RendererState* renderer_state = (RendererState*)malloc(sizeof(RendererState));
    if (!renderer_state) {
        LOGE("Failed to allocate RendererState. Exiting.");
        // Consider ANativeActivity_finish(app->activity) or similar if in a bad state
        return;
    }
    memset(renderer_state, 0, sizeof(RendererState)); // Initialize to zero

    // Assign the renderer state to app->userData so it can be accessed in callbacks.
    app->userData = renderer_state;

    // Set the callbacks for command and input events.
    app->onAppCmd = renderer_handle_cmd;
    app->onInputEvent = renderer_handle_input;

    // If the app is started with an existing saved state, try to restore it.
    // (Not fully implemented in this skeleton, but good to be aware of)
    if (app->savedState != NULL) {
        LOGI("Restoring saved state. (Size: %zu bytes)", app->savedStateSize);
        // renderer_state = (RendererState*)app->savedState; // Or copy contents
        // For now, we don't do anything with it, just log.
        // A real app would need to carefully manage this memory.
    }

    LOGI("android_main: Starting event loop.");

    // Main event loop.
    while (1) {
        int ident;
        int events;
        struct android_poll_source* source;

        // Poll for events.
        // The timeout is -1 (wait indefinitely) if not drawing, 0 if drawing.
        // This is to ensure responsiveness when active, and save power when not.
        // renderer_state->is_visible_and_focused is updated by renderer_handle_cmd.
        int timeout = (renderer_state && renderer_state->is_visible_and_focused && app->window != NULL) ? 0 : -1;
        if ((ident = ALooper_pollAll(timeout, NULL, &events, (void**)&source)) >= 0) {
            // Process this event.
            if (source != NULL) {
                source->process(app, source);
            }

            // Check if the app is exiting.
            if (app->destroyRequested != 0) {
                LOGI("android_main: App destroy requested. Terminating renderer and exiting.");
                // Ensure EGL resources are released. renderer_handle_cmd might have already
                // called renderer_term if APP_CMD_TERM_WINDOW was received.
                // Calling it again should be safe if renderer_term is idempotent or checks state.
                if (renderer_state->display != EGL_NO_DISPLAY) { // Check if term already happened
                    renderer_term(app);
                }
                // Free the RendererState
                if (app->userData) {
                    free(app->userData);
                    app->userData = NULL;
                }
                return; // Exit the main loop and thus the native application.
            }
        }

        // Draw the current frame if the window is available and the app is active/focused.
        // This check is important to prevent drawing when the surface might not be valid
        // or when the app is paused/not visible.
        if (renderer_state && renderer_state->is_visible_and_focused && app->window != NULL) {
            if (renderer_state->display != EGL_NO_DISPLAY) { // Ensure EGL is initialized
                 renderer_draw_frame(app);
            } else {
                // This case could happen if GAINED_FOCUS comes before INIT_WINDOW
                // or if there's a logic error. Usually, is_visible_and_focused
                // should only be true after renderer_init sets up the display.
                LOGW("android_main: Skipping draw_frame - EGL display not ready, though app is focused/visible.");
            }
        }
    }

    // This part should ideally not be reached if destroyRequested is handled correctly.
    LOGI("android_main: Event loop ended unexpectedly.");
    if (renderer_state && renderer_state->display != EGL_NO_DISPLAY) {
        renderer_term(app);
    }
    if (app->userData) {
        free(app->userData);
        app->userData = NULL;
    }
}
