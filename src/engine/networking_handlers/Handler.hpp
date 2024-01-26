#pragma once
#include <entt/entt.hpp>
#include <type_traits>

#include "INetworkSystem.hpp"
#include "Physics.hpp"
#include "SharedServerSettings.hpp"

class IHandler {
public:
	virtual const bool canHandle(yojimbo::Message* message) = 0;
	virtual const TestMessageType messageType() = 0;
	virtual const void handle(entt::registry& registry, PhysicsWorld* world,
							  std::unordered_map<NetworkID, entt::entity>& idToEntity, yojimbo::Message* message) = 0;
};

template <typename T, TestMessageType E>
class Handler : virtual public IHandler {
	static_assert(std::is_base_of<Message, T>::value, "T must derive from yojimbo::Message");

public:
	const bool canHandle(yojimbo::Message* message) { return message->GetType() == E; }
	const TestMessageType messageType() { return E; };
	const void handle(entt::registry& registry, PhysicsWorld* world,
					  std::unordered_map<NetworkID, entt::entity>& idToEntity, yojimbo::Message* message) {
		internalHandle(registry, world, idToEntity, (T*)message);
	};

protected:
	virtual const void internalHandle(entt::registry& registry, PhysicsWorld* world,
									  std::unordered_map<NetworkID, entt::entity>& idToEntity, T* message) = 0;
};