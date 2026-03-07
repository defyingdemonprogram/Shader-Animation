#version 330

// Output fragment color
out vec4 finalColor;

// Uniforms (set from raylib)
uniform float u_time;
uniform vec2 u_resolution;

// Hash function for generating random values
float hash12(vec2 p) {
    vec3 p3 = fract(p.xyx / 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

// Reuleaux triangle border function
vec2 reuleaux_border(float theta) {
    vec2 dir = vec2(cos(theta), sin(theta));
    float m = max(abs(dir.x), abs(dir.y));
    float t = 0.611 - 0.48 * m + 0.16 * m * m;
    return dir * t;
}

// Worley noise function
float worley(vec2 p) {
    vec2 i = floor(p);
    p -= i;

    vec2 s = step(0.0, p) * 2.0 - 1.0;
    float w = 1e9;

    for (float x = 0.0; x <= 1.0; x++) {
        for (float y = 0.0; y <= 1.0; y++) {
            vec2 d = vec2(x, y) * s;
            vec2 c = p - d - reuleaux_border(hash12(i + d) * 6.2831);
            w = min(w, dot(c, c));
        }
    }

    return 1.0 - sqrt(w);
}

void main() {
    // Normalize coordinates similar to iResolution version
    vec2 uv = gl_FragCoord.xy / u_resolution.y * 10.0;

    // Add time-based animation
    uv += u_time * 0.5;
    
    float x = worley(uv);

    // Create a colorful gradient using sine waves
    vec3 color = vec3(
        0.5 + 0.5 * sin(x * 6.0 + u_time),
        0.5 + 0.5 * sin(x * 6.0 + u_time + 2.0),
        0.5 + 0.5 * sin(x * 6.0 + u_time + 4.0)
    );

    finalColor = vec4(color, 1.0);
}