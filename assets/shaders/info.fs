// info.fs
#version 330 core
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_resolution; // Rectangle size
uniform vec2 u_origin;     // Bottom-left corner of rectangle in window space

void main() {
    // Convert fragment coordinates to local rectangle space
    vec2 fragCoord = gl_FragCoord.xy;
    vec2 localCoord = fragCoord - u_origin;
    vec2 normCoord = localCoord / u_resolution;      // [0, 1]
    vec2 uv = normCoord * 2.0 - 1.0;                  // [-1, 1]

    // Aspect correction to keep the circle uniform
    if (u_resolution.x > u_resolution.y) {
        uv.x *= u_resolution.x / u_resolution.y;
    } else {
        uv.y *= u_resolution.y / u_resolution.x;
    }

    // Rotate coordinates over time
    float angle = u_time * 0.5;
    mat2 rot = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
    uv = rot * uv;

    // Radii definitions
    float r_main = 0.9;
    float r_half = r_main * 0.5;
    float r_dot = r_half * 0.3;

    float dist = length(uv);
    float mask = step(dist, r_main);

    // Yin-Yang base: white if right of center, black otherwise
    float base = step(0.0, uv.x);  // 1.0 (white) if x >= 0, else 0.0 (black)

    // Small circle centers
    vec2 centerTop = vec2(0.0, r_half);
    vec2 centerBottom = vec2(0.0, -r_half);

    float dTop = length(uv - centerTop);
    float dBottom = length(uv - centerBottom);
   
    // Switch colors based on small circle inclusion
    if (dTop < r_half) base = 0.0;
    if (dBottom < r_half) base = 1.0;

    // Draw inner dots
    if (dTop < r_dot) base = 1.0;
    if (dBottom < r_dot) base = 0.0;

    vec3 color = vec3(base) * mask;
    FragColor = vec4(color, mask); // Smooth edge using alpha
}
