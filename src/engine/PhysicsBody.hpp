#pragma once
#ifndef PHYSICS_BODY_H
#define PHYSICS_BODY_H

#include <glm/glm.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

namespace JPH
{
    class Body;
    class VehicleConstraint;
}

typedef JPH::BodyID IDType;

enum class MotionType : uint8_t
{
    Static,
    Kinematic,
    Dynamic,
};

struct PhysicsBody
{
    IDType bodyID;
    MotionType physicsType;
    glm::vec3 position;
    glm::vec4 rotation;
    glm::vec3 scale;
    glm::vec3 velocity;

    glm::mat4 getTransform() const;
};

struct CarPhysics {
    JPH::VehicleConstraint * constraint;
};

#endif