#include <stdexcept>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include "Util.hpp"

glm::vec3 toGlm(const btVector3& v) {
    return glm::vec3(v.x(), v.y(), v.z());
}

btVector3 toBullet(const glm::vec3& v) {
    return btVector3(v.x, v.y, v.z);
}

glm::quat toGlm(const btQuaternion& q) {
    return glm::quat(q.w(), q.x(), q.y(), q.z());
}

btQuaternion toBullet(const glm::quat& q) {
    return btQuaternion(q.x, q.y, q.z, q.w);
}

glm::mat4 toGlm(const btTransform& t) {
    glm::mat4 m;
    t.getOpenGLMatrix(glm::value_ptr(m));
    return m;
}

btTransform toBullet(const glm::mat4& m) {
    btTransform t;
    t.setFromOpenGLMatrix(glm::value_ptr(m));
    return t;
}


std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open shader file: " + filename + "\n");
	}

	const size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}