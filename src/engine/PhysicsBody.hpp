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

// TODO: Change this to a 64bit id instead and map in the physics world.
typedef JPH::BodyID IDType;

enum class MotionType : uint8_t {
	Static,
	Kinematic,
	Dynamic,
};

struct PhysicsBody {
	IDType bodyID;
	MotionType physicsType;
	bool active;
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	glm::vec3 linearVelocity;
	glm::vec3 angularVelocity;

	glm::mat4 getTransform() const;
};

struct CarPhysics {
	JPH::VehicleConstraint* constraint;
};

#endif