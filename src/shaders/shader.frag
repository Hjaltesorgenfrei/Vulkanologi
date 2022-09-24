#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 1, binding = 0) uniform sampler2D texSampler[];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat uint materialIndex;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = texture(texSampler[int(materialIndex)], fragTexCoord.xy) * vec4(fragColor, 1.0); // Shitty shadows
}