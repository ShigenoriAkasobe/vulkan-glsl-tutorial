#version 450

layout(location = 0) in vec3 iPos;
layout(location = 1) in vec3 iNormal;

layout(location = 0) out vec3 vNormal;

layout(set = 0, binding = 0) uniform UBO
{
    layout(row_major) mat4 uMvp;
    layout(row_major) mat4 uModelView;
    layout(row_major) mat4 uNormalMatrix;
    vec4 uLightDir;
} ubo;

void main()
{
    gl_Position = ubo.uMvp * vec4(iPos, 1.0);
    vNormal = mat3(ubo.uNormalMatrix) * iNormal;
}
