#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"

#include "nob.h"
#include "ffmpeg.h"

#define FONT_SIZE 52
#define BACKGROUND_COLOR ColorFromHSV(120, 1.0, 1 - 0.95)
#define RENDER_WIDTH (1920 * 2)
#define RENDER_HEIGHT (1080 * 2)
#define RENDER_FPS 60
#define RENDER_DELTA_TIME (1.0f / RENDER_FPS)

typedef struct {
    Shader shader;
    const char *text;
    int timeLoc;
    int resolutionLoc;
    int originLoc;
} Info;

typedef struct {
    Font font;
    Shader shader;
    float time;
    int timeLoc;
    int resolutionLoc;
    Info info;
    size_t size;
} Plug;

static Plug *p = NULL;

static void load_resources(void) {
    p->font = LoadFontEx("./assets/fonts/iosevka-regular.ttf", FONT_SIZE, NULL, 0);
    p->shader = LoadShader(0, "./assets/shaders/growin.fs");
    p->timeLoc = GetShaderLocation(p->shader, "u_time");
    p->resolutionLoc = GetShaderLocation(p->shader, "u_resolution");

    p->info.shader = LoadShader(0, "./assets/shaders/info.fs");
    p->info.timeLoc = GetShaderLocation(p->info.shader, "u_time");
    p->info.resolutionLoc = GetShaderLocation(p->info.shader, "u_resolution");
    p->info.originLoc = GetShaderLocation(p->info.shader, "u_origin");
    p->info.text = "Made by realsanjeev";

    if (p->info.originLoc == -1) {
        TraceLog(LOG_WARNING, "SHADER: [info.fs] Uniform 'u_origin' not found");
    }
}

static void unload_resources(void) {
    UnloadFont(p->font);
    UnloadShader(p->shader);
    UnloadShader(p->info.shader);
}

void plug_reset(void) {
    p->time = 0.0f;
}

void plug_init(void) {
    if (!p) {
        p = calloc(1, sizeof(Plug));
        assert(p);
        p->size = sizeof(*p);
    }
    plug_reset();
    load_resources();

    TraceLog(LOG_INFO, "---------------------");
    TraceLog(LOG_INFO, "Initialized Plugin");
    TraceLog(LOG_INFO, "---------------------");
}

void *plug_pre_reload(void) {
    unload_resources();
    return p;
}

void plug_post_reload(void *state) {
    p = state;
    if (p->size < sizeof(*p)) {
        TraceLog(LOG_INFO, "Migrating plug state schema %zu -> %zu bytes", p->size, sizeof(*p));
        p = realloc(p, sizeof(*p));
        p->size = sizeof(*p);
    }
    load_resources();
}

void DrawWrappedText(Font font, const char *text, Rectangle bounds, float fontSize, float spacing, Color color) {
    const char *start = text;
    // For now the text is one line so aligning text and logo
    float y = bounds.y + bounds.height / 2;
    float lineHeight = fontSize + 5;

    while (*start) {
        char line[1024] = {0};
        int charsFit = 0, lastSpace = -1;

        for (int k = 0; start[k] != '\0'; ++k) {
            if (start[k] == ' ') lastSpace = k;

            char tempSub[1024];
            int subLen = k + 1;
            if ((size_t)subLen >= sizeof(tempSub)) subLen = sizeof(tempSub) - 1;
            strncpy(tempSub, start, subLen);
            tempSub[subLen] = '\0';

            if (MeasureTextEx(font, tempSub, fontSize, spacing).x > bounds.width) {
                charsFit = (lastSpace != -1) ? lastSpace : (k > 0 ? k : 1);
                break;
            } else {
                charsFit = k + 1;
            }
        }

        strncpy(line, start, charsFit);
        line[charsFit] = '\0';
        DrawTextEx(font, line, (Vector2){bounds.x, y}, fontSize, spacing, color);
        y += lineHeight;

        start += charsFit;
        while (*start == ' ') start++;
    }
}

void plug_update(float dt, float w, float h) {
    ClearBackground(BACKGROUND_COLOR);
    p->time += dt;

    float resolution[2] = {w, h};

    // Main shader
    BeginShaderMode(p->shader);
    SetShaderValue(p->shader, p->timeLoc, &p->time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(p->shader, p->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    DrawRectangle(0, 0, w, h, WHITE);
    EndShaderMode();

    // Info shader
    float padding = 10;
    Rectangle textBounds = {
        .x = w - 400 - padding,
        .y = h - 100 - padding,
        .width = 400,
        .height = 100
    };

    BeginShaderMode(p->info.shader);
    float infoResolution[2] = {textBounds.width, textBounds.height};
    float origin[2] = {textBounds.x + textBounds.height, h - (textBounds.y + textBounds.height)};

    SetShaderValue(p->info.shader, p->info.timeLoc, &p->time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(p->info.shader, p->info.resolutionLoc, infoResolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(p->info.shader, p->info.originLoc, origin, SHADER_UNIFORM_VEC2);
    DrawRectangleRec(textBounds, BLANK);
    EndShaderMode();

    // Draw overlay text
    DrawWrappedText(p->font, p->info.text, textBounds, FONT_SIZE / 2.0f, 2, RAYWHITE);
}

bool plug_finished(void) {
    return false;
}
