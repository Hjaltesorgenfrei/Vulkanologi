#include "CarSystem.hpp"

#include <cmath>
// X11 is stupid and defines None and Convex
#undef Convex
#undef None
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

// TODO: Move these to a utility class
inline glm::vec3 toGlm(JPH::RVec3 vec) {
	return glm::vec3(vec.GetX(), vec.GetY(), vec.GetZ());
}

inline glm::quat toGlm(JPH::Quat quat) {
	return glm::quat(quat.GetX(), quat.GetY(), quat.GetZ(), quat.GetW());
}

inline glm::mat4 toGlm(JPH::Mat44 mat4) {
	glm::mat4 result;
	mat4.StoreFloat4x4((JPH::Float4 *)glm::value_ptr(result));
	return result;
}

void CarSystem::update(entt::registry &registry, float delta, entt::entity ent, CarControl const &carControl,
					   CarSettings const &settings, CarPhysics &car) const {
	using namespace JPH;

	WheeledVehicleController *controller = static_cast<WheeledVehicleController *>(car.constraint->GetController());

	auto right = -carControl.desiredSteering;
	auto forward = carControl.desiredAcceleration;
	auto brake = carControl.desiredBrake;

	controller->SetDriverInput(forward, right, brake, 0.f);

	for (auto wheel : car.wheels) {
		auto [transform, wheelIndex] = registry.get<Transform, WheelIndex>(wheel);
		transform.modelMatrix =
			toGlm(car.constraint->GetWheelWorldTransform(wheelIndex.index, Vec3::sAxisX(), Vec3::sAxisY()));
		transform.modelMatrix = glm::scale(transform.modelMatrix,
										   glm::vec3(settings.wheelWidth, settings.wheelRadius, settings.wheelRadius));
	}
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
