#pragma once

#include <yojimbo.h>

#include <entt/entt.hpp>
#include <memory>

#include "networking_handlers/Handler.hpp"

typedef uint64_t NetworkID;

class NetworkServerSystem {
public:
	NetworkServerSystem(entt::registry& registry);
	~NetworkServerSystem();

	void update(entt::registry& registry, float delta);

private:
	double serverTime = 0.0;
	double accumulator = 0.0;
	double tickRateMs = (1.0 / 60.0);
	uint64_t tick = 0;
	uint8_t privateKey[yojimbo::KeyBytes] = {0};
	yojimbo::ClientServerConfig config;

	void onNetworkedConstructed(entt::registry& registry, entt::entity entity);
	void onNetworkedDestroyed(entt::registry& registry, entt::entity entity);

	// TODO: Potentially use some sort of bimap instead, could probably save some space.
	NetworkID currentId = 420;
	std::unordered_map<NetworkID, entt::entity> idToEntity;
	std::unordered_map<entt::entity, NetworkID> entityToId;

	std::unique_ptr<yojimbo::Server> server;
	std::unique_ptr<yojimbo::Adapter> adapter;
	std::vector<std::shared_ptr<IHandler>> handlers;  // A map is probably better
};