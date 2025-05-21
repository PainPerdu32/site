// egl_renderer.h
// Defines the interface for EGL rendering functions and state.

#ifndef EGL_RENDERER_H
#define EGL_RENDERER_H

#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <android/native_window.h>
#include <android_native_app_glue.h> // For android_app struct

// Forward declaration for AInputEvent
struct AInputEvent;

// Structure to hold all EGL and rendering state
typedef struct {
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLint width;
    EGLint height;
    ANativeWindow* window; // Store native window for operations like setBuffersGeometry
    int is_visible_and_focused; // Flag to control drawing in main loop

    // Shader program and VBO
    GLuint program_id;
    GLuint vbo_id;
    GLint position_attrib_loc; // For "aPosition"
    // GLuint vPositionHandle;
    // GLuint uMVPMatrixHandle;
} RendererState;

// Initializes the EGL display, surface, and context.
// Called from renderer_handle_cmd on APP_CMD_INIT_WINDOW.
void renderer_init(struct android_app* app);

// Terminates the EGL display, surface, and context.
// Called from renderer_handle_cmd on APP_CMD_TERM_WINDOW or from android_main on app exit.
void renderer_term(struct android_app* app);

// Draws a single frame.
// Called repeatedly by the main application loop or after init.
void renderer_draw_frame(struct android_app* app);

// Handles application command events (lifecycle events).
// This is the main callback set for app->onAppCmd.
void renderer_handle_cmd(struct android_app* app, int32_t cmd);

// Handles input events.
// This is the main callback set for app->onInputEvent.
// Returns 1 if the event was handled, 0 otherwise.
int32_t renderer_handle_input(struct android_app* app, AInputEvent* event);

#endif // EGL_RENDERER_H
