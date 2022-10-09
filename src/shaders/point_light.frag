#version 450

#include "utils/global_ubo.glsl"
layout(set = 0, binding = 0) uniform UniformBufferObject {
    GlobalUbo ubo;
};

layout (location = 0) in vec2 fragOffset;
layout (location = 0) out vec4 outColor;



void main() {
    float dis = sqrt(dot(fragOffset, fragOffset));
    if (dis >= 1.0) {
        discard;
    }
    outColor = vec4(ubo.lightColor.xyz, 1.0);
}