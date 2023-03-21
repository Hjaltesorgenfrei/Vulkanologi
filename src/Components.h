#pragma once

#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <memory>
#include <glm/glm.hpp>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include "curves/Curves.h"

struct Transform {
    glm::mat4 modelMatrix;
};

struct RigidBody {
    btRigidBody* body;
};

struct Sensor {
    btGhostObject* ghost;
};

struct ControlPointPtr {
    const std::shared_ptr<ControlPoint> controlPoint; 
    // If this wrapper goes out of scope we don't crash but the points will only be accessible through the owning curve.

    ControlPointPtr() : controlPoint(std::make_shared<ControlPoint>()) {}
};

#endif