// egl_renderer.c
// Implements EGL rendering logic, including EGL setup, teardown, and frame drawing.

#include "include/egl_renderer.h"
#include <EGL/eglext.h> // For EGL_OPENGL_ES3_BIT_KHR
#include <android/log.h>
#include <stdlib.h> // For malloc, free (though not strictly used yet for shaders)

#define LOG_TAG "OpenGLApp_Renderer"
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

// Helper function to log EGL error
static void log_egl_error(const char* function_name) {
    EGLint error = eglGetError();
    LOGE("%s failed with error %d (0x%x)", function_name, error, error);
}

// Shader Compilation Utility Function
static GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    if (!shader) {
        LOGE("glCreateShader() failed for type %d", type);
        return 0;
    }

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) { // infoLen includes the null terminator
            char* infoLog = (char*)malloc(infoLen);
            if (infoLog) {
                glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
                LOGE("Could not compile shader type %d:\n%s", type, infoLog);
                free(infoLog);
            }
        } else {
            LOGE("Shader compilation failed with no info log (type %d).", type);
        }
        glDeleteShader(shader);
        return 0;
    }
    LOGI("Shader compiled successfully (type %d, id %d)", type, shader);
    return shader;
}

// Shader Program Creation Utility Function
static GLuint create_shader_program(const char* vertex_shader_source, const char* fragment_shader_source) {
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
    if (!vertex_shader) {
        return 0;
    }

    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    if (!fragment_shader) {
        glDeleteShader(vertex_shader);
        return 0;
    }

    GLuint program_id = glCreateProgram();
    if (!program_id) {
        LOGE("glCreateProgram() failed.");
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return 0;
    }

    glAttachShader(program_id, vertex_shader);
    glAttachShader(program_id, fragment_shader);
    glLinkProgram(program_id);

    GLint linked = 0;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint infoLen = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = (char*)malloc(infoLen);
            if (infoLog) {
                glGetProgramInfoLog(program_id, infoLen, NULL, infoLog);
                LOGE("Could not link program:\n%s", infoLog);
                free(infoLog);
            }
        } else {
            LOGE("Program linking failed with no info log.");
        }
        glDeleteProgram(program_id);
        program_id = 0; // Set to 0 to indicate failure
    } else {
        LOGI("Shader program linked successfully (id %d)", program_id);
    }

    // Shaders are no longer needed after linking them into the program.
    glDetachShader(program_id, vertex_shader);
    glDetachShader(program_id, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program_id;
}


void renderer_init(struct android_app* app) {
    if (!app) {
        LOGE("renderer_init: android_app is null!");
        return;
    }
    RendererState* state = (RendererState*)app->userData;
    if (!state) {
        LOGE("renderer_init: RendererState in app->userData is null!");
        return;
    }
    if (!app->window) {
        LOGE("renderer_init: app->window is null!");
        return;
    }

    state->window = app->window; // Store the window
    state->program_id = 0; // Initialize
    state->vbo_id = 0;     // Initialize
    state->position_attrib_loc = -1; // Initialize

    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_NONE
    };
    const EGLint context_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_NONE
    };

    EGLDisplay display;
    EGLConfig config;
    EGLint numConfigs;
    EGLint format;
    EGLSurface surface;
    EGLContext context;
    EGLint width;
    EGLint height;

    LOGI("Initializing EGL context");

    if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        log_egl_error("eglGetDisplay");
        return;
    }
    state->display = display;

    if (!eglInitialize(display, NULL, NULL)) {
        log_egl_error("eglInitialize");
        renderer_term(app);
        return;
    }

    if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs)) {
        log_egl_error("eglChooseConfig");
        renderer_term(app);
        return;
    }
    if (numConfigs == 0) {
        LOGE("No suitable EGL config found for ES3.");
        renderer_term(app);
        return;
    }

    if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)) {
        log_egl_error("eglGetConfigAttrib(EGL_NATIVE_VISUAL_ID)");
        renderer_term(app);
        return;
    }

    if (ANativeWindow_setBuffersGeometry(app->window, 0, 0, format) < 0) {
        LOGE("ANativeWindow_setBuffersGeometry failed");
        renderer_term(app);
        return;
    }

    surface = eglCreateWindowSurface(display, config, app->window, NULL);
    if (surface == EGL_NO_SURFACE) {
        log_egl_error("eglCreateWindowSurface");
        renderer_term(app);
        return;
    }
    state->surface = surface;

    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribs);
    if (context == EGL_NO_CONTEXT) {
        log_egl_error("eglCreateContext");
        renderer_term(app);
        return;
    }
    state->context = context;

    if (!eglMakeCurrent(display, surface, surface, context)) {
        log_egl_error("eglMakeCurrent");
        renderer_term(app);
        return;
    }

    if (!eglQuerySurface(display, surface, EGL_WIDTH, &width) ||
        !eglQuerySurface(display, surface, EGL_HEIGHT, &height)) {
        log_egl_error("eglQuerySurface for width/height");
        renderer_term(app);
        return;
    }

    state->width = width;
    state->height = height;

    LOGI("EGL Initialized. Display=%p, Surface=%p, Context=%p", state->display, state->surface, state->context);
    LOGI("OpenGL ES Version: %s", glGetString(GL_VERSION));
    LOGI("GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    LOGI("EGL Surface dimensions: %dx%d", width, height);

    // Basic OpenGL ES setup
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);

    // Shader sources
    const char* vertex_shader_src =
        "#version 300 es\n"
        "layout(location = 0) in vec3 aPosition;\n"
        "void main() {\n"
        "   gl_Position = vec4(aPosition, 1.0);\n"
        "}\n";

    const char* fragment_shader_src =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "   fragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color\n"
        "}\n";

    state->program_id = create_shader_program(vertex_shader_src, fragment_shader_src);
    if (state->program_id == 0) {
        LOGE("Failed to create shader program. Rendering will not work.");
        // No need to call renderer_term(app) here, as EGL is up.
        // Subsequent draw calls will likely do nothing or error if program_id is 0.
    } else {
        glUseProgram(state->program_id);
        // `aPosition` is layout(location = 0), so explicit query is optional but good for learning
        state->position_attrib_loc = glGetAttribLocation(state->program_id, "aPosition");
        LOGI("Shader program created. Program ID: %d, aPosition location: %d", state->program_id, state->position_attrib_loc);
        // We can verify state->position_attrib_loc is 0 if layout is respected.
    }

    // Define triangle vertices
    GLfloat vertices[] = {
         0.0f,  0.5f, 0.0f, // Top
        -0.5f, -0.5f, 0.0f, // Bottom left
         0.5f, -0.5f, 0.0f  // Bottom right
    };

    glGenBuffers(1, &state->vbo_id);
    if (state->vbo_id == 0) {
        LOGE("glGenBuffers failed. VBO not created.");
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, state->vbo_id);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind after setup
        LOGI("VBO created and data buffered. VBO ID: %d", state->vbo_id);
    }

    state->is_visible_and_focused = 1;
    renderer_draw_frame(app); // Initial draw
}

void renderer_draw_frame(struct android_app* app) {
    if (!app) return;
    RendererState* state = (RendererState*)app->userData;
    if (!state || state->display == EGL_NO_DISPLAY ||
        state->surface == EGL_NO_SURFACE || state->context == EGL_NO_CONTEXT) {
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (state->program_id != 0 && state->vbo_id != 0) {
        glUseProgram(state->program_id);
        glBindBuffer(GL_ARRAY_BUFFER, state->vbo_id);

        // Assuming aPosition is at location 0 (due to layout(location=0) or queried state->position_attrib_loc)
        // The stride is 3 * sizeof(GLfloat) because each vertex has 3 components (x,y,z) and they are tightly packed.
        // The offset is (void*)0 because aPosition is the first attribute in the buffer.
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0); // Enable attribute at location 0

        glDrawArrays(GL_TRIANGLES, 0, 3); // Draw 3 vertices (1 triangle)

        glDisableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(0); // Detach the shader program
    } else {
        LOGW("Skipping draw: Program ID or VBO ID is 0.");
    }

    EGLBoolean swapped = eglSwapBuffers(state->display, state->surface);
    if (!swapped) {
        EGLint error = eglGetError();
        LOGW("eglSwapBuffers() returned error %d (0x%x)", error, error);
        // Handle specific errors like EGL_BAD_SURFACE or EGL_CONTEXT_LOST if necessary
    }
}

void renderer_term(struct android_app* app) {
    if (!app) return;
    RendererState* state = (RendererState*)app->userData;
    if (!state) return;

    LOGI("Terminating EGL context. Display=%p, Surface=%p, Context=%p", state->display, state->surface, state->context);

    // Clean up GL resources (shaders, VBOs)
    // This should be done *before* the EGL context is destroyed.
    if (state->program_id != 0) {
        glDeleteProgram(state->program_id);
        LOGI("Deleted shader program ID: %d", state->program_id);
        state->program_id = 0;
    }
    if (state->vbo_id != 0) {
        glDeleteBuffers(1, &state->vbo_id);
        LOGI("Deleted VBO ID: %d", state->vbo_id);
        state->vbo_id = 0;
    }

    if (state->display != EGL_NO_DISPLAY) {
        if (!eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
            log_egl_error("eglMakeCurrent(EGL_NO_SURFACE)");
        }
        if (state->context != EGL_NO_CONTEXT) {
            if (!eglDestroyContext(state->display, state->context)) {
                log_egl_error("eglDestroyContext");
            }
        }
        if (state->surface != EGL_NO_SURFACE) {
            if (!eglDestroySurface(state->display, state->surface)) {
                log_egl_error("eglDestroySurface");
            }
        }
        if (!eglTerminate(state->display)) {
            log_egl_error("eglTerminate");
        }
    }

    state->display = EGL_NO_DISPLAY;
    state->context = EGL_NO_CONTEXT;
    state->surface = EGL_NO_SURFACE;
    state->window = NULL;
    state->width = 0;
    state->height = 0;
    state->is_visible_and_focused = 0;
    state->position_attrib_loc = -1;
    LOGI("EGL context terminated and GL resources released.");
}

void renderer_handle_cmd(struct android_app* app, int32_t cmd) {
    if (!app || !app->userData) {
        LOGE("renderer_handle_cmd: app or app->userData is null!");
        return;
    }
    RendererState* state = (RendererState*)app->userData;

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            LOGI("APP_CMD_INIT_WINDOW received");
            if (app->window != NULL) {
                LOGI("Window handle available. Initializing renderer.");
                renderer_init(app);
            } else {
                LOGW("APP_CMD_INIT_WINDOW received but app->window is NULL!");
            }
            break;
        case APP_CMD_TERM_WINDOW:
            LOGI("APP_CMD_TERM_WINDOW received. Terminating renderer.");
            renderer_term(app);
            break;
        case APP_CMD_GAINED_FOCUS:
            LOGI("APP_CMD_GAINED_FOCUS received.");
            if (state) state->is_visible_and_focused = 1;
            break;
        case APP_CMD_LOST_FOCUS:
            LOGI("APP_CMD_LOST_FOCUS received.");
            if (state) state->is_visible_and_focused = 0;
            break;
        case APP_CMD_CONFIG_CHANGED:
            LOGI("APP_CMD_CONFIG_CHANGED received.");
            // A robust app might re-query window dimensions and update viewport/projection here.
            // For now, we assume the surface is recreated on major changes (handled by INIT/TERM_WINDOW).
            break;
        case APP_CMD_SAVE_STATE:
            LOGI("APP_CMD_SAVE_STATE received. (Not implemented)");
            // Example: app->savedState = malloc(sizeof(RendererState));
            // memcpy(app->savedState, state, sizeof(RendererState));
            // app->savedStateSize = sizeof(RendererState);
            break;
        case APP_CMD_DESTROY:
            LOGI("APP_CMD_DESTROY received. (Main loop handles final cleanup)");
            // Native resources associated with the activity are usually released here or in android_main.
            // If renderer_term hasn't been called (e.g. no TERM_WINDOW), it might be called here.
            // However, the main loop in main.c already handles calling renderer_term if destroyRequested.
            break;
        default:
            LOGI("Received unhandled APP_CMD: %d", cmd);
            break;
    }
}

int32_t renderer_handle_input(struct android_app* app, AInputEvent* event) {
    // Example:
    // if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
    //     LOGI("Motion event: X=%f, Y=%f", AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0));
    //     return 1; // Handled
    // }
    return 0; // Event not handled
}
