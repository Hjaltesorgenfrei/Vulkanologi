#pragma once

#ifndef COMPONENTS_H
#define COMPONENTS_H

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
    ControlPoint* const controlPoint;

    ControlPointPtr() : controlPoint(new ControlPoint()) 
    {}

    ~ControlPointPtr() {
        delete controlPoint;
    }

    // delete copy and move constructors and assign operators
    ControlPointPtr(ControlPointPtr const&) = delete;             // Copy construct
    ControlPointPtr(ControlPointPtr&&) = delete;                  // Move construct
    ControlPointPtr& operator=(ControlPointPtr const&) = delete;  // Copy assign
    ControlPointPtr& operator=(ControlPointPtr &&) = delete;      // Move assign
};

#endif