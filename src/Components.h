#pragma once

#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <glm/glm.hpp>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

struct Transform {
    glm::mat4 modelMatrix;
};

struct RigidBody {
    btRigidBody* body;
};

struct Sensor {
    btGhostObject* ghost;
};

#endif