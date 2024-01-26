#pragma once

#include <entt/entt.hpp>
#include <memory>

#include "INetworkSystem.hpp"
#include "networking_handlers/Handler.hpp"

class NetworkServerSystem : public INetworkSystem {
public:
	NetworkServerSystem();
	~NetworkServerSystem();

	virtual void init(entt::registry& registry) override;
	virtual void update(entt::registry& registry, PhysicsWorld* world, float delta) override;
	const virtual yojimbo::NetworkInfo& getNetworkInfo() override;

private:
	double serverTime = 0.0;
	double accumulator = 0.0;
	double tickRateMs = (1.0 / 60.0);
	uint64_t tick = 0;
	uint8_t privateKey[yojimbo::KeyBytes] = {0};
	yojimbo::ClientServerConfig config;
	yojimbo::NetworkInfo networkInfo;

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