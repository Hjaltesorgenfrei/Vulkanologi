#pragma once
#include "../Components.hpp"
#include "../DependentSystem.hpp"

struct ControllerSystem : WSystem<ControllerSystem, Writes<GamepadInput>> {
	void update(entt::registry &registry, float delta, entt::entity ent, GamepadInput &input) const;
};