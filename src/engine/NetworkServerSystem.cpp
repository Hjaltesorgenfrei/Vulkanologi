#include "NetworkServerSystem.hpp"

#include <time.h>
#include <iostream>
#include <vector>
#include "PhysicsBody.hpp"
#include "SharedServerSettings.hpp"

using namespace yojimbo;

NetworkServerSystem::NetworkServerSystem()
{
    if ( !InitializeYojimbo() )
    {
        // This should happen in a init instead, because we can't throw exceptions in constructors
        std::cout << "error: failed to initialize Yojimbo!\n";
        return;
    }
#ifdef _DEBUG
    yojimbo_log_level(YOJIMBO_LOG_LEVEL_DEBUG);
#else
    yojimbo_log_level(YOJIMBO_LOG_LEVEL_NONE);
#endif

    srand((unsigned int)time(NULL));
    ClientServerConfig config;
    server = std::make_unique<Server>(GetDefaultAllocator(), privateKey, Address("127.0.0.1", ServerPort), config, adapter, serverTime);
    server->Start(MaxClients);
    char addressString[256];
    server->GetAddress().ToString( addressString, sizeof( addressString ) );
}

NetworkServerSystem::~NetworkServerSystem()
{
    server->Stop();
    ShutdownYojimbo();
}

void NetworkServerSystem::update(entt::registry &registry, float delta)
{
    if (!server->IsRunning())
        return;

    for (int clientId = 0; clientId < MaxClients; clientId++) {
        if (!server->IsClientConnected(clientId)) {
            continue;
        }

        auto view = registry.view<PhysicsBody>(); // TODO: Split it up so we dont send too much data in one packet
        PhysicsState* message = (PhysicsState*)server->CreateMessage(clientId, PHYSICS_STATE_MESSAGE);
        const int blockSize = static_cast<int>(view.size() * sizeof(glm::vec3));
        uint8_t * block = server->AllocateBlock(clientId, blockSize);
        size_t i = 0;
        view.each([&i, &block](auto entity, auto& body) {
            glm::vec3 * data = (glm::vec3*)(block + i * sizeof(glm::vec3));
            *data = body.position;
            i++;
        });
        server->AttachBlockToMessage(clientId, message, block, blockSize);
        message->tick = static_cast<uint32_t>(serverTime);
        message->entities = static_cast<uint32_t>(view.size());

        server->SendMessage(clientId, 0, message);
    }

    server->SendPackets();

    server->ReceivePackets();

    serverTime += delta;

    server->AdvanceTime(serverTime);
}
