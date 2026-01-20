#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 mvp;
    vec4 lightDir;
    vec4 baseColor;
} ubo;

layout(location = 0) in vec3 vNormal;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = normalize(vNormal);
    vec3 l = normalize(-ubo.lightDir.xyz);
    float ndotl = max(dot(n, l), 0.0);

    vec3 lit = ubo.baseColor.rgb * ndotl;
    outColor = vec4(lit, 1.0);
}
