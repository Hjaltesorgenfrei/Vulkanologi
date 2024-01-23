#include "PhysicsHandler.hpp"

#include <glm/glm.hpp>

const void PhysicsHandler::internalHandle(entt::registry& registry, PhysicsState* state) {
	printf("tick: %d\n", state->tick);
}
