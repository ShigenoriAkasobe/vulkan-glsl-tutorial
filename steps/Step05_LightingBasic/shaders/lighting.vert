#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 mvp;
    vec4 lightDir;
    vec4 baseColor;
} ubo;

layout(location = 0) out vec3 vNormal;

void main() {
    vec2 positions[3] = vec2[](
        vec2( 0.0, -0.6),
        vec2( 0.6,  0.6),
        vec2(-0.6,  0.6)
    );

    gl_Position = ubo.mvp * vec4(positions[gl_VertexIndex], 0.0, 1.0);

    // Flat surface facing +Z (simplified for tutorial)
    vNormal = vec3(0.0, 0.0, 1.0);
}
