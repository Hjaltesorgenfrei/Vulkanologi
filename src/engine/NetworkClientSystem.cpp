#include "NetworkClientSystem.hpp"

#include <time.h>

#include <iostream>
#include <vector>

#include "Components.hpp"
#include "PhysicsBody.hpp"
#include "SharedServerSettings.hpp"
#include "networking_handlers/PhysicsHandler.hpp"

using namespace yojimbo;

NetworkClientSystem::NetworkClientSystem() {}

NetworkClientSystem::~NetworkClientSystem() {
	client->Disconnect();
	ShutdownYojimbo();
}

void NetworkClientSystem::onNetworkedConstructed(entt::registry &registry, entt::entity entity) {
	auto &network = registry.get<Networked>(entity);
	network.id = currentId++;
	idToEntity[network.id] = entity;
	entityToId[entity] = network.id;
}

void NetworkClientSystem::onNetworkedDestroyed(entt::registry &registry, entt::entity entity) {
	auto network = registry.get<Networked>(entity);
	idToEntity.erase(idToEntity.find(network.id));
	entityToId.erase(entityToId.find(entity));
}

void NetworkClientSystem::init(entt::registry &registry) {
	if (!InitializeYojimbo()) {
		// This should happen in a init instead, because we can't throw exceptions in constructors
		std::cout << "error: failed to initialize Yojimbo!\n";
		return;
	}
#ifdef _DEBUG
	yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);
#else
	yojimbo_log_level(YOJIMBO_LOG_LEVEL_NONE);
#endif
	// We only use the physics handler in clients right now and dont send anything back
	handlers.emplace_back(std::make_shared<PhysicsHandler>());
	registry.on_construct<Networked>().connect<&NetworkClientSystem::onNetworkedConstructed>(this);
	registry.on_destroy<Networked>().connect<&NetworkClientSystem::onNetworkedDestroyed>(this);

	srand((unsigned int)time(NULL));
	// TODO: Move config to a central place so there is no duplication.
	config.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
	config.channel[1].type = CHANNEL_TYPE_RELIABLE_ORDERED;
	adapter = std::make_unique<PhysicsNetworkAdapter>();
	Address serverAddress("127.0.0.1", ServerPort);
	uint64_t clientId = 0;
	yojimbo_random_bytes((uint8_t *)&clientId, 8);
	memset(privateKey, 0, KeyBytes);

	client = std::make_unique<Client>(GetDefaultAllocator(), Address("0.0.0.0"), config, *adapter, clientTime);
	client->InsecureConnect(privateKey, clientId, serverAddress);
	if (!client->IsConnected()) {
		std::cout << "Client failed to connect\n";
	}
}

void NetworkClientSystem::update(entt::registry &registry, float delta) {
	clientTime += delta;
	accumulator += delta;

	while (accumulator >= tickRateMs) {
		accumulator -= tickRateMs;
		tick++;
		if (!client->IsConnected()) return;

		client->SendPackets();

		client->ReceivePackets();

		for (int clientId = 0; clientId < MaxClients; clientId++) {
			while (auto message = client->ReceiveMessage(0)) {
				for (auto h : handlers) {
					if (h->canHandle(message)) {
						h->handle(registry, message);
					}
				}
				client->ReleaseMessage(message);
			}
		}

		client->AdvanceTime(clientTime);
	}
}
