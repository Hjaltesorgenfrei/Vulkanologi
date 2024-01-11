#pragma once

#include <yojimbo.h>

#include <entt/entt.hpp>
#include <memory>

class NetworkServerSystem {
public:
	NetworkServerSystem();
	~NetworkServerSystem();

	void update(entt::registry& registry, float delta);

private:
	double serverTime = 0.0;
	double accumulator = 0.0;
	double tickRateMs = (1.0 / 60.0);
	uint64_t tick = 0;
	uint8_t privateKey[yojimbo::KeyBytes] = {0};
	yojimbo::ClientServerConfig config;
	std::unique_ptr<yojimbo::Server> server;
	std::unique_ptr<yojimbo::Adapter> adapter;
};