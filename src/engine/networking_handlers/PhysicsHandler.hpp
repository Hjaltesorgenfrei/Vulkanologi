#include "Handler.hpp"

class PhysicsHandler : public Handler<PhysicsState, PHYSICS_STATE_MESSAGE> {
	const void internalHandle(entt::registry& registry, PhysicsState* state);
};