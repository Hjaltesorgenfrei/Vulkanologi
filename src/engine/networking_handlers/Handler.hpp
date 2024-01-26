#pragma once
#include <type_traits>
#include <entt/entt.hpp>
#include "SharedServerSettings.hpp"

class IHandler {
public:
	virtual const bool canHandle(yojimbo::Message* message) = 0;
	virtual const TestMessageType messageType() = 0;
	virtual const void handle(entt::registry& registry, yojimbo::Message* message) = 0;
};

template <typename T, TestMessageType E>
class Handler : virtual public IHandler {
	static_assert(std::is_base_of<Message, T>::value, "T must derive from yojimbo::Message");

public:
	const bool canHandle(yojimbo::Message* message) { return message->GetType() == E; }
	const TestMessageType messageType() { return E; };
	const void handle(entt::registry& registry, yojimbo::Message* message) { internalHandle(registry, (T*)message); };

protected:
	virtual const void internalHandle(entt::registry& registry, T* message) = 0;
};