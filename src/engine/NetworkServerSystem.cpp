#include "NetworkServerSystem.hpp"

#include <time.h>

#include <iostream>
#include <vector>

#include "PhysicsBody.hpp"
#include "SharedServerSettings.hpp"
#include "networking_handlers/PhysicsHandler.hpp"
#include "Components.hpp"

using namespace yojimbo;

NetworkServerSystem::NetworkServerSystem(entt::registry &registry) {
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

	srand((unsigned int)time(NULL));
	// TODO: Move config to a central place so there is no duplication.
	config.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
	config.channel[1].type = CHANNEL_TYPE_RELIABLE_ORDERED;
	adapter = std::make_unique<PhysicsNetworkAdapter>();
	server = std::make_unique<Server>(GetDefaultAllocator(), privateKey, Address("127.0.0.1", ServerPort), config,
									  *adapter, serverTime);
	server->Start(MaxClients);
	if (!server->IsRunning()) {
		std::cout << "Server failed to start\n";
	}
	registry.on_construct<Networked>().connect<&NetworkServerSystem::onNetworkedConstructed>(this);
	registry.on_destroy<Networked>().connect<&NetworkServerSystem::onNetworkedDestroyed>(this);
}

NetworkServerSystem::~NetworkServerSystem() {
	server->Stop();
	ShutdownYojimbo();
}

void NetworkServerSystem::onNetworkedConstructed(entt::registry &registry, entt::entity entity) {
	auto &network = registry.get<Networked>(entity);
	network.id = currentId++;
	idToEntity[network.id] = entity;
	entityToId[entity] = network.id;
}

void NetworkServerSystem::onNetworkedDestroyed(entt::registry &registry, entt::entity entity) {
	auto network = registry.get<Networked>(entity);
	idToEntity.erase(idToEntity.find(network.id));
	entityToId.erase(entityToId.find(entity));
}

auto path = "lol/car.obj";

void NetworkServerSystem::update(entt::registry &registry, float delta) {
	serverTime += delta;
	accumulator += delta;

	while (accumulator >= tickRateMs) {
		accumulator -= tickRateMs;
		tick++;
		if (!server->IsRunning()) return;

		for (int clientId = 0; clientId < MaxClients; clientId++) {
			if (!server->IsClientConnected(clientId)) {
				continue;
			}

			for (auto [entity, body] : registry.view<PhysicsBody>().each()) {
				PhysicsState *message = (PhysicsState *)server->CreateMessage(clientId, PHYSICS_STATE_MESSAGE);
				message->tick = tick;
				message->position.x = body.position.x;
				message->position.y = body.position.y;
				message->position.z = body.position.z;

				message->rotation.x = body.rotation.x;
				message->rotation.y = body.rotation.y;
				message->rotation.z = body.rotation.z;
				message->rotation.w = body.rotation.w;

				message->velocity.x = body.velocity.x;
				message->velocity.y = body.velocity.y;
				message->velocity.z = body.velocity.z;
				server->SendMessage(clientId, 0, message);
			}

			continue;
			// Currently unused, network ids are hardcoded
			auto spawnMessage = (CreateGameObject *)server->CreateMessage(clientId, CREATE_GAME_OBJECT_MESSAGE);
			spawnMessage->tick = tick;
			spawnMessage->hasMesh = true;
			spawnMessage->hasPhysics = false;
			spawnMessage->physicsSettings.size = 42;
			spawnMessage->isClientOwned = false;
			spawnMessage->isPlayerControlled = true;
			strcpy_s(spawnMessage->meshPath, path);
			spawnMessage->meshPathLength = static_cast<uint32_t>(strlen(path));
			server->SendMessage(clientId, 0, spawnMessage);
		}

		server->SendPackets();

		server->ReceivePackets();

		for (int clientId = 0; clientId < MaxClients; clientId++) {
			while (auto message = server->ReceiveMessage(clientId, 0)) {
				for (auto h : handlers) {
					if (h->canHandle(message)) {
						h->handle(registry, message);
					}
				}
				server->ReleaseMessage(clientId, message);
			}
		}

		server->AdvanceTime(serverTime);
	}
}
