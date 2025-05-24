#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"

#define FONT_SIZE 52
#define BACKGROUND_COLOR ColorFromHSV(120, 1.0, 1 - 0.95)
#define RENDER_WIDTH (1920 * 2)
#define RENDER_HEIGHT (1080 * 2)

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
    Shader shader;
    int timeLoc;
    int resolutionLoc;
    int textureLoc;
    Texture2D envTexture;
} DragonBall;

typedef struct {
    Font font;
    
    DragonBall db;
    float time;
    Info info;
    size_t size;
} Plug;

static Plug *p = NULL;

static void load_resources(void) {
    p->font = LoadFontEx("./assets/fonts/iosevka-regular.ttf", FONT_SIZE, NULL, 0);
    p->db.shader = LoadShader(0, "./assets/shaders/dragonball.fs");
    p->db.timeLoc = GetShaderLocation(p->db.shader, "time");
    p->db.resolutionLoc = GetShaderLocation(p->db.shader, "resolution");
    p->db.textureLoc = GetShaderLocation(p->db.shader, "texture1");

    // Load environment
    p->db.envTexture = LoadTexture("./assets/textures/environment.png");

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
    UnloadShader(p->db.shader);
    UnloadTexture(p->db.envTexture);
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

void plug_update(float dt, float w, float h, bool  render) {
    ClearBackground(BACKGROUND_COLOR);
    p->time += dt;

    float resolution[2] = {w, h};

    // Main shader
    BeginShaderMode(p->db.shader);
        SetShaderValue(p->db.shader, p->db.timeLoc, &p->time, SHADER_UNIFORM_FLOAT);
        SetShaderValue(p->db.shader, p->db.resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
        SetShaderValueTexture(p->db.shader, p->db.textureLoc, p->db.envTexture);

        // Scale environment.png to screen dimensions
        Rectangle src = { 0, 0, (float)p->db.envTexture.width, (float)p->db.envTexture.height };
        Rectangle dest = { 0, 0, w, h };
        Vector2 originDb = { 0, 0 };
        DrawTexturePro(p->db.envTexture, src, dest, originDb, 0.0f, WHITE);

    EndShaderMode();

    // if (render) {
    //     // Info shader
    //     float p->info.padding = 10;
    //     Rectangle p->info.textBounds = {
    //         .x = w - 400 - p->info.padding,
    //         .y = h - 100 - p->info.padding,
    //         .width = 400,
    //         .height = 100
    //     };

     // Info shader
    if (!render) {
        p->info.fontSize = FONT_SIZE / 2;
        p->info.padding = 10;
        p->info.textBounds = CLITERAL(Rectangle){
            .x = w - 400 - p->info.padding,
            .y = h - 100 - p->info.padding,
            .width = 400,
            .height = 100
        };
    } else {
        p->info.fontSize = w / 70;
        p->info.padding = w / 192;
        p->info.textBounds = CLITERAL(Rectangle){
            .x = w - w / 5 - p->info.padding,
            .y = h - h / 11 - p->info.padding,
            .width = w / 5,
            .height = h / 11
        };
    }

    BeginShaderMode(p->info.shader);
        float infoResolution[2] = {p->info.textBounds.width, p->info.textBounds.height};
        float origin[2] = {p->info.textBounds.x + p->info.textBounds.height, h - (p->info.textBounds.y + p->info.textBounds.height)};

        SetShaderValue(p->info.shader, p->info.timeLoc, &p->time, SHADER_UNIFORM_FLOAT);
        SetShaderValue(p->info.shader, p->info.resolutionLoc, infoResolution, SHADER_UNIFORM_VEC2);
        SetShaderValue(p->info.shader, p->info.originLoc, origin, SHADER_UNIFORM_VEC2);
        DrawRectangleRec(p->info.textBounds, BLANK);
    EndShaderMode();

    // Draw overlay text
    DrawWrappedText(p->font, p->info.text, p->info.textBounds, p->info.fontSize, 2, RAYWHITE);
}

bool plug_finished(void) {
    return false;
}
