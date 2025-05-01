#include <stdlib.h>
#include <assert.h>

#include "raylib.h"
#include "raymath.h"

#include "nob.h"
#include "ffmpeg.h"

#define FONT_SIZE 52
#define BACKGROUND_COLOR ColorFromHSV(120, 1.0, 1 - 0.95)

#define RENDER_WIDTH 1920
#define RENDER_HEIGHT 1080
#define RENDER_FPS 60
#define RENDER_DELTA_TIME (1.0f/RENDER_FPS)

#define script_size NOB_ARRAY_LEN(script)

typedef struct {
    Font font;
    Shader shader;
    float time;
    int timeLoc;
    size_t size;
} Plug;

static Plug *p = NULL;

static void load_resources(void) {
    p->font = LoadFontEx("./assets/fonts/iosevka-regular.ttf", FONT_SIZE, NULL, 0);
    p->shader = LoadShader(0, "./assets/shader/example.fs");
    p->timeLoc = GetShaderLocation(p->shader, "u_time");
}

static void unload_resources(void) {
    UnloadFont(p->font);
    UnloadShader(p->shader);
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

void plug_update(float dt, float w, float h) {
    ClearBackground(BACKGROUND_COLOR);
    p->time += dt;
    BeginShaderMode(p->shader);
    SetShaderValue(p->shader, p->timeLoc, &p->time, SHADER_UNIFORM_FLOAT);

    DrawRectangle(0, 0, w, h, WHITE);
    EndShaderMode();
    const char *text = "Imagination is more important than knowledge. For knowledge is limited, whereas imagination embraces the entire world.” — Albert Einstein";
    
    float maxWidth = w * 0.8f;
    float textX = w * 0.1f;
    float textY = h * 0.3f;
    Rectangle bounds = { textX, textY, maxWidth, h };
    
    DrawWrappedText(p->font, text, bounds, FONT_SIZE, 1.0f, BLACK);
}

bool plug_finished(void) {
    return false;
}
