#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <float.h>

#include "raylib.h"
#include "raymath.h"

#include "nob.h"
#include "ffmpeg.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define FONT_SIZE 52
#define BACKGROUND_COLOR ColorFromHSV(120, 1.0, 1 - 0.95)
#define RENDER_WIDTH (1920)
#define RENDER_HEIGHT (1080)
#define TEXTURE_WIDTH (RENDER_WIDTH / 6)
#define TEXTURE_HEIGHT (RENDER_HEIGHT / 6)
#define RENDER_FPS 30
#define RENDER_DELTA_TIME (1.0f / RENDER_FPS)

typedef struct {
    Font font;
    Shader shader;
    const char *text;

    // Shader input variables
    int timeLoc;
    int resolutionLoc;
    int originLoc;
} Info;

typedef struct {
    Shader shader;
    int timeLoc;
    int resolutionLoc;
    int texture0Loc;
} SmoothLife;

typedef struct {
    RenderTexture2D state[2];

    float time;
    size_t size;
    size_t currentState;

    SmoothLife sl;
    Info info;
} Plug;

static Plug *p = NULL;

static float rand_float(void) {
    return (float)rand() / (float)RAND_MAX;
}

static void generateNoiseImage(Image *image) {
    Color *pixels = (Color *)image->data;
    for (int i = 0; i < image->width * image->height; i++) {
        uint8_t value = (uint8_t)(rand_float() * 255.0f);
        pixels[i] = (Color){ value, value, value, 255 };
    }
}

static void load_resources(void) {
    assert(p);

    p->sl.shader = LoadShader(NULL, "./assets/shaders/smoothlife.fs");
    if (!p->sl.shader.id) TraceLog(LOG_ERROR, "Failed to load smoothlife shader.");
    p->sl.resolutionLoc = GetShaderLocation(p->sl.shader, "resolution");
    p->sl.timeLoc = GetShaderLocation(p->sl.shader, "dt");
    p->sl.texture0Loc = GetShaderLocation(p->sl.shader, "texture0");

    p->info.font = LoadFontEx("./assets/fonts/iosevka-regular.ttf", FONT_SIZE, NULL, 0);
    p->info.shader = LoadShader(NULL, "./assets/shaders/info.fs");
    if (!p->info.shader.id) TraceLog(LOG_ERROR, "Failed to load info shader.");
    p->info.timeLoc = GetShaderLocation(p->info.shader, "u_time");
    p->info.resolutionLoc = GetShaderLocation(p->info.shader, "u_resolution");
    p->info.originLoc = GetShaderLocation(p->info.shader, "u_origin");
    p->info.text = "Made by realsanjeev";

    if (p->info.originLoc == -1) {
        TraceLog(LOG_WARNING, "SHADER: [info.fs] Uniform 'u_origin' not found");
    }

    // Image image = GenImageColor(TEXTURE_WIDTH, TEXTURE_HEIGHT, WHITE);
    Image image = GenImagePerlinNoise(TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, 0, 2.0f);
    // generateNoiseImage(&image);

    p->state[0] = LoadRenderTexture(TEXTURE_WIDTH, TEXTURE_HEIGHT);
    SetTextureWrap(p->state[0].texture, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(p->state[0].texture, TEXTURE_FILTER_POINT);
    UpdateTexture(p->state[0].texture, image.data);

    p->state[1] = LoadRenderTexture(TEXTURE_WIDTH, TEXTURE_HEIGHT);
    SetTextureWrap(p->state[1].texture, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(p->state[1].texture, TEXTURE_FILTER_POINT);

    UnloadImage(image);
}

static void unload_resources(void) {
    assert(p);
    UnloadShader(p->sl.shader);
    UnloadFont(p->info.font);
    UnloadShader(p->info.shader);
    UnloadRenderTexture(p->state[0]);
    UnloadRenderTexture(p->state[1]);
}

void plug_reset(void) {
    if (!p) return;
    p->time = 0.0f;
    p->currentState = 0;
}

void plug_init(void) {
    if (!p) {
        p = (Plug *)calloc(1, sizeof(Plug));
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
    if (!p) return NULL;
    unload_resources();
    return p;
}

void plug_post_reload(void *state) {
    p = (Plug *)state;
    if (p->size < sizeof(*p)) {
        TraceLog(LOG_INFO, "Migrating plug state schema %zu -> %zu bytes", p->size, sizeof(*p));
        p = realloc(p, sizeof(*p));
        p->size = sizeof(*p);
    }
    load_resources();
    TraceLog(LOG_INFO, "Plugin Reloaded");
}

void DrawWrappedText(const Font font, const char *text, Rectangle bounds, float fontSize, float spacing, Color color) {
    const char *start = text;
    // For now the text is one line so aligning text and logo
    float y = bounds.y + bounds.height / 2.0f;
    float lineHeight = fontSize + 5.0f;

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
        DrawTextEx(font, line, (Vector2){ bounds.x, y }, fontSize, spacing, color);
        y += lineHeight;

        start += charsFit;
        while (*start == ' ') start++;
    }
}

void plug_update(float dt, float w, float h) {
    // ClearBackground(BACKGROUND_COLOR);
    float smoothLifedt = (dt <= FLT_EPSILON) ? 0.0f : RENDER_DELTA_TIME;
    p->time += dt;
    printf("Time: %f\n", smoothLifedt);

    float slResolution[2] = { (float)TEXTURE_WIDTH, (float)TEXTURE_HEIGHT };

    // Run simulation shader
    BeginTextureMode(p->state[1 - p->currentState]);
        BeginShaderMode(p->sl.shader);
            // Set shader inputs
            SetShaderValueTexture(p->sl.shader, p->sl.texture0Loc, p->state[p->currentState].texture);
            SetShaderValue(p->sl.shader, p->sl.resolutionLoc, slResolution, SHADER_UNIFORM_VEC2);
            SetShaderValue(p->sl.shader, p->sl.timeLoc, &smoothLifedt, SHADER_UNIFORM_FLOAT);
            DrawTexture(p->state[p->currentState].texture, 0, 0, WHITE);
        EndShaderMode();
    EndTextureMode();

    // Swap states
    p->currentState = 1 - p->currentState;

    // Draw to screen
    // ClearBackground(BACKGROUND_COLOR);
    float scale = MIN(w / TEXTURE_WIDTH, h / TEXTURE_HEIGHT);
    Vector2 offset = { (w - TEXTURE_WIDTH * scale) / 2.0f, (h - TEXTURE_HEIGHT * scale) / 2.0f };
    DrawTextureEx(p->state[1 - p->currentState].texture, offset, 0.0f, scale, WHITE);
    // DrawTextureEx(p->state[p->currentState].texture, (Vector2){0, 0}, 0.0f, w / TEXTURE_WIDTH, WHITE);

    // Overlay info text
    float padding = 10;
    Rectangle textBounds = {
        .x = w - 400 - padding,
        .y = h - 100 - padding,
        .width = 400,
        .height = 100
    };

    BeginShaderMode(p->info.shader);
        float infoResolution[2] = { textBounds.width, textBounds.height };
        float origin[2] = { textBounds.x + textBounds.height, h - (textBounds.y + textBounds.height) };

        SetShaderValue(p->info.shader, p->info.timeLoc, &p->time, SHADER_UNIFORM_FLOAT);
        SetShaderValue(p->info.shader, p->info.resolutionLoc, infoResolution, SHADER_UNIFORM_VEC2);
        SetShaderValue(p->info.shader, p->info.originLoc, origin, SHADER_UNIFORM_VEC2);
        DrawRectangleRec(textBounds, BLANK);
    EndShaderMode();

    // Draw overlay text
    DrawWrappedText(p->info.font, p->info.text, textBounds, FONT_SIZE / 2.0f, 2.0f, YELLOW);
}

bool plug_finished(void) {
    return false;
}
