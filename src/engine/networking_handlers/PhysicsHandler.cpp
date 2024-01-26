#include "PhysicsHandler.hpp"

#include <glm/glm.hpp>

glm::vec3 fromFloat3(float3 v3) {
	return glm::vec3(v3.x, v3.y, v3.z);
}

glm::quat fromFloat4(float4 v4) {
	return glm::quat(v4.x, v4.y, v4.z, v4.w);
}

const void PhysicsHandler::internalHandle(entt::registry& registry, PhysicsWorld* world,
										  std::unordered_map<NetworkID, entt::entity>& idToEntity,
										  PhysicsState* state) {
	auto entity = idToEntity[state->networkID];
	auto body = registry.try_get<PhysicsBody>(entity);
	
	printf("Set body %i!\n", state->networkID);

	if (body) {
		world->setBodyPosition(body->bodyID, fromFloat3(state->position));
		world->setBodyVelocity(body->bodyID, fromFloat3(state->velocity));
		world->setBodyRotation(body->bodyID, fromFloat4(state->rotation));
	}
}
