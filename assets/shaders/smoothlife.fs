#version 330 core

// Input vertex attributes (from vertex shaders)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec2 resolution;
uniform float dt;

// Output fragment color
out vec4 finalColor;

float ra = 21.0;
float b1 = 0.257;
float b2 = 0.336;
float d1 = 0.365;
float d2 = 0.549;
float alpha_n = 0.028;
float alpha_m = 0.147;

// float dt = 0.0;
#define PI 3.14159265359

float sigma(float x, float a, float alpha) {
    return 1.0 / (1.0 + exp(-(x - a) * 4.0 / alpha));
}

float sigma_n(float x, float a, float b) {
    return sigma(x, a, alpha_n) * (1.0 - sigma(x, b, alpha_n));
}

float sigma_m(float x, float y, float m) {
    return x * (1.0 - sigma(m, 0.5, alpha_m)) + y * sigma(m, 0.5, alpha_m);
}

float s(float n, float m) {
    return sigma_n(n, sigma_m(b1, d1, m), sigma_m(b2, d2, m));
}

float grid(float x, float y) {
    vec2 uv = vec2(mod(x, resolution.x), mod(y, resolution.y)) / resolution;
    vec4 texColor = texture(texture0, uv);
    return max(max(texColor.x, texColor.y), texColor.z);
}

void main() {
    float cx = fragTexCoord.x * resolution.x;
    float cy = (1.0 - fragTexCoord.y) * resolution.y;

    float ri = ra / 3.0;
    float m = 0.0;
    float M = PI * ri * ri;
    float n = 0.0;
    float N = PI * ra * ra - M;

    // Sample in a circular region
    for (float dy = -ra; dy <= ra; dy += 1.0) {
        for (float dx = -ra; dx <= ra; dx += 1.0) {
            float x = cx + dx;
            float y = cy + dy;
            float dist_sq = dx * dx + dy * dy;
            if (dist_sq <= ri * ri) {
                m += grid(x, y);
            } else if (dist_sq <= ra * ra) {
                n += grid(x, y);
            }
        }
    }
    m /= M;
    n /= N;
    float q = s(n, m);
    float diff = 2.0 * q - 1.0;
    float v = clamp(grid(cx, cy) + dt * diff, 0.0, 1.0);
    // finalColor = fragColor;// vec4(v, v, v, 1.0)*fragColor.x;
    finalColor = vec4(v, v, v, 1.0);
}
