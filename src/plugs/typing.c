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
    size_t size;

    Info info;
} Plug;

static Plug *p = NULL;

static void load_resources(void) {
    p->font = LoadFontEx("./assets/fonts/iosevka-regular.ttf", FONT_SIZE, NULL, 0);
    p->shader = LoadShader(0, "./assets/shaders/example.fs");
    p->timeLoc = GetShaderLocation(p->shader, "u_time");


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
        // Only if u_resolution is present in example.fs, but let's assume it might be or just use time
        DrawRectangle(0, 0, (int)w, (int)h, WHITE);
    EndShaderMode();

    const char *fullText = "Everything is judged by its appearance; what is unseen counts for nothing. Never let yourself get lost in the crowd or buried in oblivion. Stand out. Be conspicuous, at all cost! Make yourself a magnet of attention by appearing larger, more colorful, more mysterious, than the bland and timid masses.";
    
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
    if (!render) {
        p->info.fontSize = FONT_SIZE / 2;
        p->info.padding = 10;
        p->info.textBounds = (Rectangle){
            .x = w - 400 - p->info.padding,
            .y = h - 100 - p->info.padding + FONT_SIZE,
            .width = 400,
            .height = 100
        };
    } else {
        p->info.fontSize = w / 35;
        p->info.padding = w / 192;
        p->info.textBounds = (Rectangle){
            .x = w - w / 4 - p->info.padding,
            .y = h - h / 11 - p->info.padding,
            .width = w / 4,
            .height = h / 11
        };
    }

    BeginShaderMode(p->info.shader);
        float infoResolution[2] = { (float)p->info.textBounds.width, (float)p->info.textBounds.height };
        float origin[2] = { (float)(p->info.textBounds.x + p->info.textBounds.height + FONT_SIZE), (float)(h - (p->info.textBounds.y + p->info.textBounds.height - FONT_SIZE)) };

        SetShaderValue(p->info.shader, p->info.timeLoc, &p->time, SHADER_UNIFORM_FLOAT);
        SetShaderValue(p->info.shader, p->info.resolutionLoc, infoResolution, SHADER_UNIFORM_VEC2);
        SetShaderValue(p->info.shader, p->info.originLoc, origin, SHADER_UNIFORM_VEC2);
        DrawRectangleRec(p->info.textBounds, BLANK);
    EndShaderMode();

    DrawWrappedText(p->font, p->info.text, p->info.textBounds, p->info.fontSize, 2, RAYWHITE);
}

bool plug_finished(void) {
    return false;
}
