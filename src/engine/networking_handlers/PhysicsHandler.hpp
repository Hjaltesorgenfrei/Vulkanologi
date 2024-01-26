#include "Handler.hpp"

class PhysicsHandler : public Handler<PhysicsState, PHYSICS_STATE_MESSAGE> {
	const void internalHandle(entt::registry& registry, PhysicsWorld* world,
							  std::unordered_map<NetworkID, entt::entity>& idToEntity, PhysicsState* state);
};