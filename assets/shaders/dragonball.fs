#version 330

// Uniforms for resolution and time (passed from Raylib)
uniform vec2 resolution;
uniform float time;
uniform sampler2D texture1;

// Input from vertex shader
in vec2 fragTexCoord;

// Output color
out vec4 finalColor;

const float PI = 3.14159265359;

// Scene function: defines a sphere with radius (height)
float scene(vec3 position) {
    float height = 0.3;
    return length(position) - height;
}

// Calculate normal at a point on the surface
vec3 getNormal(vec3 pos, float smoothness) {	
    vec3 n;
    vec2 dn = vec2(smoothness, 0.0);
    n.x = scene(pos + dn.xyy) - scene(pos - dn.xyy);
    n.y = scene(pos + dn.yxy) - scene(pos - dn.yxy);
    n.z = scene(pos + dn.yyx) - scene(pos - dn.yyx);
    return normalize(n);
}

// Raymarching function
float raymarch(vec3 position, vec3 direction) {
    float total_distance = 0.0;
    for (int i = 0; i < 32; ++i) {
        float result = scene(position + direction * total_distance);
        if (result < 0.005) {
            return total_distance;
        }
        total_distance += result;
    }
    return -1.0;
}

// Look-at matrix for camera orientation
mat3 calcLookAtMatrix(vec3 ro, vec3 ta, float roll) {
    vec3 ww = normalize(ta - ro);
    vec3 uu = normalize(cross(ww, vec3(sin(roll), cos(roll), 0.0)));
    vec3 vv = normalize(cross(uu, ww));
    return mat3(uu, vv, ww);
}

void main() {
    // Adjust UV coordinates to match Shadertoy behavior
    vec2 uv = gl_FragCoord.xy / resolution.y;
    uv -= vec2(0.5 * resolution.x / resolution.y, 0.5);
    uv.y *= -1.0;

    // Camera position orbiting around origin
    vec3 origin = vec3(sin(time * 0.1) * 2.5, 0.0, cos(time * 0.1) * 2.5);

    // Camera look-at matrix
    mat3 camMat = calcLookAtMatrix(origin, vec3(0.0), 0.0);
    vec3 direction = normalize(camMat * vec3(uv, 2.5));

    // Raymarch to find intersection
    float dist = raymarch(origin, direction);

    if (dist < 0.0) {
        // Background: sample environment texture
        vec2 dirUV = vec2(atan(direction.z, direction.x) / (2.0 * PI) + 0.5, acos(direction.y) / PI);
        finalColor = texture(texture1, dirUV);
        // finalColor = texture(texture1, direction.xy);
        // finalColor = vec4(direction, dist);
    } else {
        // Hit the sphere
        vec3 fragPosition = origin + direction * dist;
        vec3 N = getNormal(fragPosition, 0.01);
        vec4 ballColor = vec4(1.0, 0.8, 0.0, 1.0) * 0.75;
        vec3 ref = reflect(direction, N);

        // Star effect
        float P = PI / 5.0;
        float starVal = (1.0 / P) * (P - abs(mod(atan(uv.x, uv.y) + PI, (2.0 * P)) - P));
        vec4 starColor = (distance(uv, vec2(0.0, 0.0)) < 0.06 - (starVal * 0.03)) ? vec4(2.8, 1.0, 0.0, 1.0) : vec4(0.0);

        // Rim lighting
        float rim = max(0.0, (0.7 + dot(N, direction)));

        // Refraction
        vec3 refr = refract(direction, N, 0.7);

        // Map refracted and reflected directions to 2D UVs
        vec2 refrUV = vec2(atan(refr.z, refr.x) / (2.0 * PI) + 0.5, acos(refr.y) / PI);
        vec2 refUV = vec2(atan(ref.z, ref.x) / (2.0 * PI) + 0.5, acos(ref.y) / PI);
        
        // Remap uv to [0,1] for the glow effect
        vec2 uv2D = uv + 0.5;

        // Combine colors
    //     finalColor = 
    //         texture(texture1, refr.xy) * ballColor +
    //         (vec4(0.6, 0.2, 0.0, 1.0) * max(0.0, 1.0 - distance(uv * 4.0, vec2(0.0, 0.0)))) * 4.0 * (0.2 + abs(sin(time)) * 0.8) +
    //         starColor +
    //         texture(texture1, ref.xy) * 0.3 +
    //         vec4(rim, rim * 0.5, 0.0, 1.0);
    // }
    finalColor = 
            texture(texture1, refrUV) * ballColor +
            (vec4(0.6, 0.2, 0.0, 1.0) * max(0.0, 1.0 - distance(uv2D * 4.0, vec2(0.5, 0.5)))) * 4.0 * (0.2 + abs(sin(time)) * 0.8) +
            starColor +
            texture(texture1, refUV) * 0.3 +
            vec4(rim, rim * 0.5, 0.0, 1.0);
    }
}