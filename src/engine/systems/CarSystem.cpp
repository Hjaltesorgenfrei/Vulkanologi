#include "CarSystem.hpp"

#include <cmath>
// X11 is stupid and defines None and Convex
#undef Convex
#undef None
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>

#include <glm/gtc/quaternion.hpp>

inline glm::vec3 toGlm(JPH::RVec3 vec) {
	return glm::vec3(vec.GetX(), vec.GetY(), vec.GetZ());
}

inline glm::quat toGlm(JPH::Quat quat) {
	return glm::quat(quat.GetX(), quat.GetY(), quat.GetZ(), quat.GetW());
}

void CarSystem::update(entt::registry &registry, float delta, entt::entity ent, CarControl const &carControl,
					   CarPhysics &car) const {
	using namespace JPH;

	WheeledVehicleController *controller = static_cast<WheeledVehicleController *>(car.constraint->GetController());

	auto right = -carControl.desiredSteering;
	auto forward = carControl.desiredAcceleration;
	auto brake = carControl.desiredBrake;

	controller->SetDriverInput(forward, right, brake, 0.f);
}

void CarKeyboardSystem::update(entt::registry &registry, float delta, entt::entity ent, KeyboardInput const &input,
							   CarControl &carControl) const {
	carControl.desiredAcceleration = 0.0f;
	carControl.desiredBrake = 0.0f;

	if (input.keys[GLFW_KEY_RIGHT]) {
		carControl.desiredSteering = -1.f;
	} else if (input.keys[GLFW_KEY_LEFT]) {
		carControl.desiredSteering = 1.f;
	} else {
		carControl.desiredSteering = 0.0f;
	}

	if (input.keys[GLFW_KEY_UP]) {
		carControl.desiredAcceleration = 1.0f;
	} else if (input.keys[GLFW_KEY_DOWN]) {
		carControl.desiredAcceleration = -1.0f;
	} else {
		carControl.desiredAcceleration = 0.0f;
	}

	if (input.keys[GLFW_KEY_SPACE]) {
		carControl.desiredBrake = 1.0f;
	}
}

void CarJoystickSystem::update(entt::registry &registry, float delta, entt::entity ent, GamepadInput const &input,
							   CarControl &carControl) const {
	carControl.desiredAcceleration = 0.0f;
	carControl.desiredBrake = 0.0f;

	carControl.desiredSteering = -(input.leftStick.x);
	// If we are going forwards, the right trigger controls acceleration
	carControl.desiredAcceleration += input.rightTrigger * 1.0f;
	carControl.desiredAcceleration += -input.leftTrigger * 1.0f;
	carControl.desiredAcceleration = std::clamp(carControl.desiredAcceleration, -1.0f, 1.0f);

	// TODO: Right now you have to stop completely before you can reverse. fix this
}
