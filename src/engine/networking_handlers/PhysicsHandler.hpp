#include <queue>
#include <chrono>

#include "Handler.hpp"

const uint32_t BUFFER_DELAY = 5;

class PhysicsHandler : public Handler<PhysicsState, PHYSICS_STATE_MESSAGE> {
public:
	void update(entt::registry& registry, PhysicsWorld* world, std::unordered_map<NetworkID, entt::entity>& idToEntity) override;
protected:
	const void internalHandle(entt::registry& registry, PhysicsWorld* world,
							  std::unordered_map<NetworkID, entt::entity>& idToEntity, PhysicsState* state);
	std::map<uint32_t, std::queue<PhysicsState*>>  buffer;
	float time;
};