#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <yojimbo.h>

using namespace yojimbo;

const int ClientPort = 40000;
const int ServerPort = 50000;

struct float3 {
	float x;
	float y;
	float z;
};

struct float4 {
	float x;
	float y;
	float z;
	float w;
};

struct PhysicsState : public Message {
	uint32_t tick;
	float3 position;
	float4 rotation;
	float3 velocity;

	PhysicsState() { tick = 0; }

	template <typename Stream>
	bool Serialize(Stream& stream) {
		serialize_bits(stream, tick, 32);
		serialize_float(stream, position.x);
		serialize_float(stream, position.y);
		serialize_float(stream, position.z);

		serialize_float(stream, rotation.x);
		serialize_float(stream, rotation.y);
		serialize_float(stream, rotation.z);
		serialize_float(stream, rotation.w);

		serialize_float(stream, velocity.x);
		serialize_float(stream, velocity.y);
		serialize_float(stream, velocity.z);

		return true;
	}

	YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct CreateGameObject : public Message {
	uint32_t tick;
	bool isPlayerControlled;
	bool isClientOwned;
	bool hasPhysics;
	struct PhysicsSettings {
		int size;
	} physicsSettings;
	bool hasMesh;
	char meshPath[128];
	uint32_t meshPathLength;

	template <typename Stream>
	bool Serialize(Stream& stream) {
		serialize_bits(stream, tick, 32);
		serialize_bool(stream, hasMesh);
		serialize_bool(stream, hasPhysics);
		if (hasPhysics) {
			serialize_int(stream, physicsSettings.size, 0, 511);
		}
		serialize_bool(stream, isPlayerControlled);
		serialize_bool(stream, isClientOwned);
		serialize_string(stream, meshPath, 128);
		return true;
	}

	YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

// TODO: This should be a enum class, shorter names and namespaced
enum TestMessageType { PHYSICS_STATE_MESSAGE, CREATE_GAME_OBJECT_MESSAGE, NUM_TEST_MESSAGE_TYPES };

// TODO: Fix some naming here, it is not only physics.
YOJIMBO_MESSAGE_FACTORY_START(PhysicsMessageFactory, NUM_TEST_MESSAGE_TYPES);
YOJIMBO_DECLARE_MESSAGE_TYPE(PHYSICS_STATE_MESSAGE, PhysicsState);
YOJIMBO_DECLARE_MESSAGE_TYPE(CREATE_GAME_OBJECT_MESSAGE, CreateGameObject);
YOJIMBO_MESSAGE_FACTORY_FINISH();

class PhysicsNetworkAdapter : public Adapter {
public:
	MessageFactory* CreateMessageFactory(Allocator& allocator) {
		return YOJIMBO_NEW(allocator, PhysicsMessageFactory, allocator);
	}
};
