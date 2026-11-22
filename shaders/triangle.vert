#version 450

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[3](
    vec2(0.0, -0.6),
    vec2(0.6, 0.6),
    vec2(-0.6, 0.6)
);

vec3 colors[3] = vec3[3](
    vec3(1.0, 0.25, 0.75),
    vec3(0.0, 0.9, 1.0),
    vec3(0.5, 0.1, 1.0)
);

void main() {
    fragColor = colors[gl_VertexIndex];
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
