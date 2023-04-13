#include <cmath>
#include "CarSystem.hpp"
// X11 is stupid and defines None and Convex
#undef Convex
#undef None
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <glm/gtc/quaternion.hpp>

inline glm::vec3 toGlm(JPH::RVec3 vec) {
    return glm::vec3(vec.GetX(), vec.GetY(), vec.GetZ());
}

inline glm::quat toGlm(JPH::Quat quat) {
    return glm::quat(quat.GetX(), quat.GetY(), quat.GetZ(), quat.GetW());
}

void CarSystem::update(entt::registry &registry, float delta, entt::entity ent, CarControl const &carControl, CarPhysics &car, CarStateLastUpdate &lastState) const
{
    float	sMaxEngineTorque = 500.0f;
    float	sClutchStrength = 10.0f;
    bool	sLimitedSlipDifferentials = true;

    using namespace JPH;

    WheeledVehicleController *controller = static_cast<WheeledVehicleController *>(car.constraint->GetController());

	// Update vehicle statistics
	controller->GetEngine().mMaxTorque = sMaxEngineTorque;
	controller->GetTransmission().mClutchStrength = sClutchStrength;

	// Set slip ratios to the same for everything
	float limited_slip_ratio = sLimitedSlipDifferentials? 1.4f : FLT_MAX;
	controller->SetDifferentialLimitedSlipRatio(limited_slip_ratio);
	for (VehicleDifferentialSettings &d : controller->GetDifferentials())
		d.mLimitedSlipRatio = limited_slip_ratio;

	// Pass the input on to the constraint
    // TODO: swap steering direction
    auto right = -carControl.desiredSteering;
    auto forward = carControl.desiredAcceleration;
    auto brake = carControl.desiredBrake;

	controller->SetDriverInput(forward, right, brake, 0.f);


    // auto currentSteering = car.vehicle->getSteeringValue(0);
    // auto currentAcceleration = car.vehicle->getWheelInfo(2).m_engineForce;
    // auto currentBrake = car.vehicle->getWheelInfo(2).m_brake;
    

    // float desiredSteering = std::clamp(steering, -car.maxSteering, car.maxSteering);
    // float desiredAcceleration = std::clamp(acceleration, -(car.maxAcceleration * 0.5f), car.maxAcceleration);
    // float desiredBrake = std::clamp(brake, 0.0f, car.maxBrake);

    // car.steering = glm::mix(currentSteering, desiredSteering, 0.02f * delta);
    // car.acceleration = glm::mix(currentAcceleration, desiredAcceleration, 0.04f * delta);
    // car.brake = glm::mix(currentBrake, desiredBrake, 0.03f * delta);

    // car.vehicle->applyEngineForce(car.acceleration, 2);
    // car.vehicle->applyEngineForce(car.acceleration, 3);
    // car.vehicle->setSteeringValue(car.steering, 0);
    // car.vehicle->setSteeringValue(car.steering, 1);
    // car.vehicle->setBrake(car.brake, 0);
    // car.vehicle->setBrake(car.brake, 1);
    // car.vehicle->setBrake(car.brake, 2);
    // car.vehicle->setBrake(car.brake, 3);

    // lastState.speed = car.vehicle->getCurrentSpeedKmHour();
    // lastState.direction = toGlm(car.vehicle->getForwardVector());
    // lastState.position = toGlm(car.vehicle->getChassisWorldTransform().getOrigin());
}

void CarKeyboardSystem::update(entt::registry &registry, float delta, entt::entity ent, KeyboardInput const &input, CarStateLastUpdate const &lastState, CarControl &carControl) const
{
    carControl.desiredAcceleration = 0.0f;
    carControl.desiredBrake = 0.0f;

    if (input.keys[GLFW_KEY_RIGHT]) {
        carControl.desiredSteering = -1.f;
    }
    else if (input.keys[GLFW_KEY_LEFT]) {
        carControl.desiredSteering = 1.f;
    }
    else {
        carControl.desiredSteering = 0.0f;
    }

    if (input.keys[GLFW_KEY_UP]) {
        carControl.desiredAcceleration = 1.0f;
    }
    else if (input.keys[GLFW_KEY_DOWN]) {
        carControl.desiredAcceleration = -1.0f;
    }
    else {
        carControl.desiredAcceleration = 0.0f;
    }

    if (lastState.speed >= 15.0f && input.keys[GLFW_KEY_DOWN]) {
        carControl.desiredBrake = 1.0f;
    }
    else if(lastState.speed <= -15.0f && input.keys[GLFW_KEY_UP]) {
        carControl.desiredBrake = 1.0f;
    }

    if (input.keys[GLFW_KEY_SPACE]) {
        carControl.desiredBrake = 1.0f;
    }
}

void CarJoystickSystem::update(entt::registry &registry, float delta, entt::entity ent, GamepadInput const &input, CarStateLastUpdate const &lastState, CarControl &carControl) const
{
    carControl.desiredAcceleration = 0.0f;
    carControl.desiredBrake = 0.0f;

    carControl.desiredSteering = -(input.leftStick.x);
    // If we are going forwards, the right trigger controls acceleration
    carControl.desiredAcceleration += input.rightTrigger * 1.0f;
    carControl.desiredAcceleration += -input.leftTrigger * 1.0f;
    carControl.desiredAcceleration = std::clamp(carControl.desiredAcceleration, -1.0f, 1.0f);


    if (lastState.speed >= 15.0f) {
        carControl.desiredBrake = input.leftTrigger * 1.0f;
    }
    else if(lastState.speed <= -15.0f) {
        carControl.desiredBrake = input.rightTrigger * 1.0f;
    }

    // TODO: Right now you have to stop completely before you can reverse. fix this
}
