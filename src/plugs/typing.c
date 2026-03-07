#include <stdlib.h>
#include <assert.h>

#include "raylib.h"
#include "raymath.h"

#include "nob.h"
#include "ffmpeg.h"

#define BACKGROUND_COLOR ColorFromHSV(120, 1.0, 1 - 0.95)

#define FONT_SIZE 52
#define TYPING_SPEED 30.0f

// Struct for the rendering the LOGO
typedef struct {
    Shader shader;
    int timeLoc;
    int resolutionLoc;
    int originLoc;

    const char *text;
    size_t fontSize;
    float padding;
    Rectangle textBounds;
} Info;

typedef struct {
    Font font;
    Shader shader;
    float time;
    int timeLoc;
    int resolutionLoc;
    size_t size;

    Info info;
} Plug;

static Plug *p = NULL;

static void load_resources(void) {
    p->font = LoadFontEx("./assets/fonts/iosevka-regular.ttf", FONT_SIZE, NULL, 0);
    p->shader = LoadShader(0, "./assets/shaders/hash_without_sine.fs");
    p->timeLoc = GetShaderLocation(p->shader, "u_time");
    p->resolutionLoc = GetShaderLocation(p->shader, "u_resolution");


    // Load the shader for logo rendering
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

void plug_reset(void)
{
    p->time = 0.0f;
}

void plug_init(void) {
    if (!p) {
        p = calloc(1, sizeof(Plug));
        assert(p != NULL); // Ensure allocation succeeded
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
        TraceLog(LOG_INFO, "Migrating plug state schema %zu bytes -> %zu bytes", p->size, sizeof(*p));
        p = realloc(p, sizeof(*p));
        p->size = sizeof(*p);
    }
    load_resources();
}

void DrawWrappedText(Font font, const char *text, Rectangle bounds, float fontSize, float spacing, Color color) {
    const char *start = text;
    float lineHeight = fontSize + 5;
    float y = bounds.y;

    while (*start) {
        size_t len = strlen(start);
        char line[1024] = {0};
        int i = 0;
        float width = 0;

        // Try to fit as many words as possible into a line
        while (start[i] && width < bounds.width &&i < (int)(sizeof(line) - 1)) {
            line[i] = start[i];
            line[i + 1] = '\0';
            Vector2 size = MeasureTextEx(font, line, fontSize, spacing);
            width = size.x;

            if (width >= bounds.width) {
                // backtrack to last space
                while (i > 0 && line[i] != ' ') i--;
                line[i] = '\0';
                break;
            }
            i++;
        }
        DrawTextEx(font, line, (Vector2){bounds.x, y}, fontSize, spacing, color);
        y += lineHeight;
        start += i;
        while(*start == ' ') start++; // skip leading spaces
    }
}

void plug_update(float dt, float w, float h, bool  render) {
    (void)render;
    ClearBackground(BACKGROUND_COLOR);
    p->time += dt;

    // Background shader
    BeginShaderMode(p->shader);
        float resolution[2] = { w, h };
        SetShaderValue(p->shader, p->timeLoc, &p->time, SHADER_UNIFORM_FLOAT);
        SetShaderValue(p->shader, p->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
        DrawRectangle(0, 0, (int)w, (int)h, WHITE);
    EndShaderMode();

    const char *fullText = "Everything is judged by its appearance; what is unseen counts for nothing. Never let yourself get lost in the crowd or buried in oblivion. Stand out. Be conspicuous, at all cost! Make yourself a magnet of attention by appearing larger, more colorful, more mysterious, than the bland and timid masses.\n\n\n- Robert Greene, The 48 Laws of Power";
    
    // Typing effect logic
    int totalLen = strlen(fullText);
    int charsToDisplay = (int)(p->time * TYPING_SPEED);
    if (charsToDisplay > totalLen) charsToDisplay = totalLen;

    char *displayText = (char *)malloc(charsToDisplay + 1);
    if (displayText) {
        strncpy(displayText, fullText, charsToDisplay);
        displayText[charsToDisplay] = '\0';

        float maxWidth = w * 0.8f;
        float textX = w * 0.1f;
        float textY = h * 0.1f;
        Rectangle bounds = { textX, textY, maxWidth, h };
        
        DrawWrappedText(p->font, displayText, bounds, FONT_SIZE, 1.0f, RAYWHITE);
        free(displayText);
    }

    // Info/Logo logic
    float padding;
    float font_size;
    Rectangle logoBounds;
    // TODO(realsanjeev): Use the minimum of the w and h to calculate padding and font_size
    if (render) {
        padding = w / 108;
        logoBounds = (Rectangle){
            .x = w - w / 2.5 - padding,
            .y = h - h / 17 - padding,
            .width = w / 2.3,
            .height = h / 17
        };
        font_size = (w / 30);
    } else {
        w = GetScreenWidth();
        h = GetScreenHeight();
        padding = w / 108;

        logoBounds = (Rectangle){
            .x = w - w / 2.7 - padding,
            .y = h - h / 17 - padding,
            .width = w / 2.7,
            .height = h / 17
        };
        font_size = (w / 40);
    }

    BeginShaderMode(p->info.shader);
        float logoResolution[2] = {logoBounds.width, logoBounds.height};
        float origin[2] = {logoBounds.x + logoBounds.height, h - (logoBounds.y + logoBounds.height)};

        SetShaderValue(p->info.shader, p->info.timeLoc, &p->time, SHADER_UNIFORM_FLOAT);
        SetShaderValue(p->info.shader, p->info.resolutionLoc, logoResolution, SHADER_UNIFORM_VEC2);
        SetShaderValue(p->info.shader, p->info.originLoc, origin, SHADER_UNIFORM_VEC2);
        DrawRectangleRec(logoBounds, BLANK);
    EndShaderMode();

    // Draw overlay text
    // Change the logoBounds location to align with loGo
    Rectangle textBounds = {
        .x = logoBounds.x - padding*3,
        .y = logoBounds.y + font_size,
        .width = logoBounds.width,
        .height = logoBounds.height
    };
    DrawWrappedText(p->font, p->info.text, textBounds, font_size, 2, RAYWHITE);
}

bool plug_finished(void) {
    return false;
}
