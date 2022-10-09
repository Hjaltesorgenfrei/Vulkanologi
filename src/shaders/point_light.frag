#version 450

layout (location = 0) in vec2 fragOffset;
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 projView;
    vec4 ambientLightColor; // w is intensity
    vec4 lightPosition; // w is ignored
    vec4 lightColor; // w is intensity
} ubo;

void main() {
    float dis = sqrt(dot(fragOffset, fragOffset));
    if (dis >= 1.0) {
        discard;
    }
    outColor = vec4(ubo.lightColor.xyz, 1.0);
}