#pragma once
#ifndef PHYSICS_BODY_H
#define PHYSICS_BODY_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace JPH {
class Body;
class VehicleConstraint;
}  // namespace JPH

typedef JPH::BodyID IDType;

enum class MotionType : uint8_t {
	Static,
	Kinematic,
	Dynamic,
};

struct PhysicsBody {
	IDType bodyID;
	MotionType physicsType;
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	glm::vec3 velocity;

	glm::mat4 getTransform() const;
};

struct CarPhysics {
	JPH::VehicleConstraint* constraint;
};

#endif