#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 mvp;
} ubo;

void main() {
    vec2 positions[3] = vec2[](
        vec2( 0.0, -0.6),
        vec2( 0.6,  0.6),
        vec2(-0.6,  0.6)
    );

    gl_Position = ubo.mvp * vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
