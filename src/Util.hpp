#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <btBulletDynamicsCommon.h>

enum Axis {
    X,
    Y,
    Z
};

glm::vec3 toGlm(const btVector3& v);
btVector3 toBullet(const glm::vec3& v);
glm::quat toGlm(const btQuaternion& q);
btQuaternion toBullet(const glm::quat& q);
glm::mat4 toGlm(const btTransform& t);
btTransform toBullet(const glm::mat4& m);

std::vector<char> readFile(const std::string& filename);