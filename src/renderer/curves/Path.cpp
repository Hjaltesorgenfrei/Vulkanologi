#include "Path.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vulkan/vulkan.hpp>

using namespace glm;

Path::Path(bool loop) : loop(loop) {
	points = std::vector<Point>();
	indices = std::vector<uint32_t>();
}

std::vector<uint32_t> const &Path::getIndices() const {
	return indices;
}

std::vector<Point> const &Path::getPoints() const {
	return points;
}

std::vector<FrenetFrame> const &Path::getFrenetFrames() const {
	return frenetFrames;
}

void Path::recompute() {}

mat4 frenetFrameMatrix(FrenetFrame frame) {
	auto direction = frame.t;
	auto up = frame.n;
	auto pos = frame.o;
	auto z = normalize(direction);
	auto x = normalize(cross(up, z));
	auto y = normalize(cross(z, x));
	return mat4(x.x, x.y, x.z, 0.f, y.x, y.y, y.z, 0.f, z.x, z.y, z.z, 0.f, pos.x, pos.y, pos.z, 1.f);
}

quat RotationBetweenVectors(vec3 start, vec3 dest) {
	using namespace glm;
	start = normalize(start);
	dest = normalize(dest);

	float cosTheta = dot(start, dest);
	vec3 rotationAxis;

	if (cosTheta < -1 + 0.001f) {
		// special case when vectors in opposite directions :
		// there is no "ideal" rotation axis
		// So guess one; any will do as long as it's perpendicular to start
		// This implementation favors a rotation around the Up axis,
		// since it's often what you want to do.
		rotationAxis = cross(vec3(0.0f, 0.0f, 1.0f), start);
		if (length2(rotationAxis) < 0.01)  // bad luck, they were parallel, try again!
			rotationAxis = cross(vec3(1.0f, 0.0f, 0.0f), start);

		rotationAxis = normalize(rotationAxis);
		return angleAxis(radians(180.0f), rotationAxis);
	}

	// Implementation from Stan Melax's Game Programming Gems 1 article
	rotationAxis = cross(start, dest);

	float s = sqrt((1 + cosTheta) * 2);
	float invs = 1 / s;

	return quat(s * 0.5f, rotationAxis.x * invs, rotationAxis.y * invs, rotationAxis.z * invs);
}

mat4 FrenetFrame::rotation() const {
	vec3 direction = normalize(r);
	quat rot1 = RotationBetweenVectors(vec3(0.0f, 0.0f, -1.0f), direction);
	vec3 newUp = rot1 * vec3(0.0f, 1.0f, 0.0f);
	quat rot2 = RotationBetweenVectors(newUp, n);
	return mat4_cast(rot2 * rot1);
}

vec3 FrenetFrame::binormal() const {
	return normalize(cross(t, n));
}

std::vector<vk::VertexInputBindingDescription> Point::getBindingDescriptions() {
	return {vk::VertexInputBindingDescription{
		.binding = 0, .stride = sizeof(Point), .inputRate = vk::VertexInputRate::eVertex}};
}

std::vector<vk::VertexInputAttributeDescription> Point::getAttributeDescriptions() {
	return {
		vk::VertexInputAttributeDescription{
			.location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Point, position)},
		vk::VertexInputAttributeDescription{
			.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Point, normal)},
		vk::VertexInputAttributeDescription{
			.location = 2, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Point, color)}};
}