#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
	mat4 projView;
	vec4 ambientLightColor; // w is intensity
	vec4 lightPosition; // w is ignored
	vec4 lightColor; // w is intensity
} ubo;

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
	outColor = texture(texSampler[int(materialIndex)], fragTexCoord.xy) * vec4((diffuseLight + ambientLight) * fragColor, 1.0);
}