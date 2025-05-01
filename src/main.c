#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "raylib.h"

#include <dlfcn.h>

#define NOB_IMPLEMENTATION
#include "nob.h"
#include "plug.h"
#include "ffmpeg.h"

#define RENDER_WIDTH 1920*2
#define RENDER_HEIGHT 1080*2
#define RENDER_FPS 60
#define RENDER_DELTA_TIME (1.0f/RENDER_FPS)
#define RENDERING_FONT_SIZE 78

// The state of Panim Engine
static bool paused = false;
static FFMPEG *ffmpeg = NULL;
static RenderTexture2D screen = {0};
static Font rendering_font = {0};
static void *libplug = NULL;

static bool reload_libplug(const char *libplug_path) {
    if (libplug != NULL) {
        dlclose(libplug);
    }

    libplug = dlopen(libplug_path, RTLD_NOW);
    if (libplug == NULL) {
        fprintf(stderr, "ERROR: %s\n", dlerror());
        return false;
    }

    #define PLUG(name, ...) \
        name = dlsym(libplug, #name); \
        if (name == NULL) { \
            fprintf(stderr, "ERROR: %s\n", dlerror()); \
            return false; \
        }
    LIST_OF_PLUGS
    #undef PLUG

    return true;
}

static void finish_ffmpeg_rendering(bool cancel) {
    SetTraceLogLevel(LOG_INFO);
    ffmpeg_end_rendering(ffmpeg, cancel);
    plug_reset();
    ffmpeg = NULL;
}

void rendering_scene(const char *text) {
    const char *sub_text = "Press Esc to Cancel.";
    Color foreground_color = ColorFromHSV(0, 0, 0.95);
    Color background_color = ColorFromHSV(0, 0, 0.05);

    ClearBackground(background_color);
    Vector2 text_size = MeasureTextEx(rendering_font, text, RENDERING_FONT_SIZE, 0);
    Vector2 position = {
        GetScreenWidth()/2 - text_size.x/2,
        GetScreenHeight()/2 - text_size.y/2,
    };
    DrawTextEx(rendering_font, text, position, RENDERING_FONT_SIZE, 0, foreground_color);

    // Subtext rendering
    float sub_font_size = RENDERING_FONT_SIZE / 2.0f;
    Vector2 sub_size = MeasureTextEx(rendering_font, sub_text, sub_font_size, 0);
    Vector2 sub_position = {
        GetScreenWidth()/2 - sub_size.x/2,
        position.y + text_size.y
    };
    DrawTextEx(rendering_font, sub_text, sub_position, sub_font_size, 0, foreground_color);

    // Bouncing dots
    float circle_radius = RENDERING_FONT_SIZE * 0.2f;
    float ball_height = GetScreenHeight() * 0.03;
    float ball_padding = GetScreenHeight() * 0.02;
    float waving_speed = 2;
    float ball_min_y = position.y + RENDERING_FONT_SIZE + sub_size.y + ball_padding;

    {
        Vector2 center = {
            .x = position.x + text_size.x * 0.5f - circle_radius * 3,
            .y = ball_min_y + ball_height * (sinf(GetTime() * waving_speed - PI / 4) + 1) * 0.5f,
        };
        DrawCircleV(center, circle_radius, foreground_color);
    }

    {
        Vector2 center = {
            .x = position.x + text_size.x * 0.5f,
            .y = ball_min_y + ball_height * (sinf(GetTime() * waving_speed) + 1) * 0.5f,
        };
        DrawCircleV(center, circle_radius, foreground_color);
    }

    {
        Vector2 center = {
            .x = position.x + text_size.x * 0.5f + circle_radius * 3,
            .y = ball_min_y + ball_height * (sinf(GetTime() * waving_speed + PI / 4) + 1) * 0.5f,
        };
        DrawCircleV(center, circle_radius, foreground_color);
    }
}

int main(int argc, char **argv) {
    const char *program_name = nob_shift_args(&argc, &argv);

    if (argc <= 0) {
        fprintf(stderr, "Usage: %s <libplug.so>\n", program_name);
        fprintf(stderr, "ERROR: no animation dynamic library is provided\n");
        return 1;
    }

    const char *libplug_path = nob_shift_args(&argc, &argv);
        
    if (!reload_libplug(libplug_path)) return 1;

    float scale_factor = 100.0f;
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(16*scale_factor, 9*scale_factor, "Panim");
    InitAudioDevice();
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    plug_init();

    screen = LoadRenderTexture(RENDER_WIDTH, RENDER_HEIGHT);
    rendering_font = LoadFontEx("./assets/fonts/Vollkorn-Regular.ttf", RENDERING_FONT_SIZE, NULL, 0);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_K)) {
            TraceLog(LOG_INFO, "Exiting on K press");
            break;
        }
        
        BeginDrawing();
            if (ffmpeg) {
                if (plug_finished()) {
                    finish_ffmpeg_rendering(false);
                } else if (IsKeyPressed(KEY_ESCAPE)) {
                    finish_ffmpeg_rendering(false);
                } else {
                    BeginTextureMode(screen);
                    plug_update(RENDER_DELTA_TIME, RENDER_WIDTH, RENDER_HEIGHT, true);
                    EndTextureMode();

                    Image image = LoadImageFromTexture(screen.texture);
                    if (!ffmpeg_send_frame_flipped(ffmpeg, image.data, image.width, image.height)) {
                        finish_ffmpeg_rendering(true);
                    }
                    UnloadImage(image);
                }
                rendering_scene("Rendering Video");
            } else {
                if (IsKeyPressed(KEY_R)) {
                    SetTraceLogLevel(LOG_WARNING);
                    ffmpeg = ffmpeg_start_rendering(RENDER_WIDTH, RENDER_HEIGHT, RENDER_FPS);
                    plug_reset();
                } else {
                    if (IsKeyPressed(KEY_H)) {
                        void *state = plug_pre_reload();
                        reload_libplug(libplug_path);
                        plug_post_reload(state);
                    }

                    if (IsKeyPressed(KEY_SPACE)) {
                        paused = !paused;
                    }

                    if (IsKeyPressed(KEY_Q)) {
                        plug_reset();
                    }
                    plug_update(paused ? 0.0f : GetFrameTime(), GetScreenWidth(), GetScreenHeight(), false);
                }
            }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}

