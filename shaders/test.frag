#version 450

layout(location = 0) in vec3 vNrm;
layout(location = 1) in vec3 vColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = normalize(vNrm);
    vec3 l = normalize(vec3(-0.3, -1.0, -0.2));
    float ndotl = max(dot(n, -l), 0.0);

    vec3 lit = vColor * (0.15 + 0.85 * ndotl);
    outColor = vec4(lit, 1.0);
}
