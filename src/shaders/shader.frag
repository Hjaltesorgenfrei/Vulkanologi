#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "utils/global_ubo.glsl"
layout(set = 0, binding = 0) uniform UniformBufferObject {
	GlobalUbo ubo;
};

layout (push_constant) uniform constants {
	mat4 modelMatrix;
	vec4 color;
} pushConstants;

layout(set = 1, binding = 0) uniform sampler2D texSampler[];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat uint materialIndex;
layout(location = 3) in vec3 fragPosWorld;
layout(location = 4) in vec3 fragNormalWorld;

layout(location = 0) out vec4 outColor;

void main() {
	vec3 directionToLight = ubo.lightPosition.xyz - fragPosWorld;
	float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared

	vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w * attenuation;
	vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	vec3 diffuseLight = lightColor * max(dot(normalize(fragNormalWorld), normalize(directionToLight)), 0);
	// Darken the color if it is not facing up, dirty hack to make the ground look better while i dont have shadows
	vec4 color = texture(texSampler[int(materialIndex)], fragTexCoord.xy) * vec4((diffuseLight + ambientLight) * fragColor * pushConstants.color.xyz, 1.0);
	color *= clamp(dot(normalize(fragNormalWorld), vec3(0.0, 1.0, 0.0)), 0.7, 1.0);
	color *= clamp(dot(normalize(fragNormalWorld), vec3(0.0, 0.0, -1.0)), 0.85, 1.0);
	outColor = color;
}