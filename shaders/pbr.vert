#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 vColor;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec3 eye;
} ubo;

layout(push_constant) uniform Push {
    mat4 model;
} pc;

void main() {
    vec4 worldPos = vec4(inPosition, 1.0);
    gl_Position = ubo.viewProj * worldPos;

    vNormal = inNormal;
    
    vColor = inColor;
}
