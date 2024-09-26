#pragma once
#include "../Components.hpp"
#include "../DependentSystem.hpp"

struct ControllerSystem : public ISystem {
	const std::string_view name();
	const std::unordered_set<std::type_index> reads();
	const std::unordered_set<std::type_index> writes();
	void update(entt::registry &registry, float delta) const;
};