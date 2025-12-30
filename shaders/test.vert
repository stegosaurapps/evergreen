#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNrm;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 vNrm;
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
    // vec4 worldPos = pc.model * vec4(inPos, 1.0);
    // gl_Position = ubo.viewProj * worldPos;

    // vNrm = mat3(pc.model) * inNrm;
    
    vec4 worldPos = vec4(inPos, 1.0);
    gl_Position = ubo.viewProj * worldPos;

    vNrm = inNrm;
    
    vColor = inColor;
}
