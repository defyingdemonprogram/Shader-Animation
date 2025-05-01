#version 330 core

out vec4 FragColor;
uniform float u_time;

void main() {
    float r = 0.5 + 0.5 * sin(u_time);
    float g = 0.5 + 0.5 * sin(u_time + 2.0);
    float b = 0.5 + 0.5 * sin(u_time + 4.0);
    FragColor = vec4(r, g, b, 1.0);
}
