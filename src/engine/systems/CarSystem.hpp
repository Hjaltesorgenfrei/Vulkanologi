#pragma once
#include "../Components.hpp"
#include "../DependentSystem.hpp"
#include "../PhysicsBody.hpp"

struct CarSystem : System<CarSystem, Reads<CarControl, CarSettings>, Writes<CarPhysics>, Others<>> {
	void update(entt::registry &registry, float delta, entt::entity ent, CarControl const &carControl,
				CarSettings const &settings, CarPhysics &car) const;
};

struct CarKeyboardSystem : System<CarKeyboardSystem, Reads<KeyboardInput>, Writes<CarControl>, Others<>> {
	void update(entt::registry &registry, float delta, entt::entity ent, KeyboardInput const &input,
				CarControl &carControl) const;
};

struct CarJoystickSystem : System<CarJoystickSystem, Reads<GamepadInput>, Writes<CarControl>, Others<>> {
	void update(entt::registry &registry, float delta, entt::entity ent, GamepadInput const &input,
				CarControl &carControl) const;
};