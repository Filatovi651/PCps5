#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pc;

vec3 positions[3] = vec3[3](
    vec3(0.0, -0.5, 0.0),
    vec3(0.5, 0.5, 0.0),
    vec3(-0.5, 0.5, 0.0)
);

vec3 colors[3] = vec3[3](
    vec3(1.0, 0.25, 0.75),
    vec3(0.0, 0.9, 1.0),
    vec3(0.5, 0.1, 1.0)
);

layout(location = 0) out vec3 fragColor;

void main() {
    fragColor = colors[gl_VertexIndex];
    gl_Position = pc.mvp * vec4(positions[gl_VertexIndex], 1.0);
}
