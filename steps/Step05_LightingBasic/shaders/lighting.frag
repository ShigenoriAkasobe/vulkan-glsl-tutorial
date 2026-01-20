#version 450

layout(location = 0) in vec3 vNormal;
layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 0) uniform UBO
{
    mat4 uMvp;
    mat4 uModelView;
    mat4 uNormalMatrix;
    vec4 uLightDir;
} ubo;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-ubo.uLightDir.xyz);
    float ndotl = max(dot(N, L), 0.0);

    vec3 baseColor = vec3(0.8, 0.6, 0.4);
    vec3 ambient = baseColor * 0.2;
    vec3 diffuse = baseColor * ndotl * 0.8;
    vec3 color = ambient + diffuse;

    oColor = vec4(color, 1.0);
}
