#pragma once

#include <entt/entt.hpp>
#include <yojimbo.h>

typedef uint64_t NetworkID;

// This file is a nasty nasty dirty copy and should be deleted after the hackathon
// TODO: Delete this file. Or at least gut it and rework it a bunch
class INetworkSystem {
public:
	virtual void init(entt::registry& registry) = 0;
	virtual void update(entt::registry& registry, float delta) = 0;
	const virtual yojimbo::NetworkInfo& getNetworkInfo() = 0;
};