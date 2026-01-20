#version 450

layout(location = 0) in vec3 iPos;
layout(location = 1) in vec3 iColor;

layout(location = 0) out vec3 vColor;

layout(set = 0, binding = 0) uniform UBO
{
    layout(row_major) mat4 uMvp;  // C++ uses row-major format
} ubo;

void main()
{
    gl_Position = ubo.uMvp * vec4(iPos, 1.0);
    vColor = iColor;
}
