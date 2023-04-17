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
    double serverTime = 100.0;
    uint8_t privateKey[yojimbo::KeyBytes] = { 0 };
    std::unique_ptr<yojimbo::Server> server;
};