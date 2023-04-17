#pragma once

#include <yojimbo.h>
#include <stdint.h>
#include <inttypes.h>

using namespace yojimbo;

const int ClientPort = 40000;
const int ServerPort = 50000;

struct PhysicsState : public BlockMessage
{
    uint32_t tick;
    uint32_t entities;

    PhysicsState()
    {
        tick = 0;
        entities = 0;
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {        
        serialize_bits( stream, tick, 32 );
        serialize_bits( stream, entities, 32 );
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

enum TestMessageType
{
    PHYSICS_STATE_MESSAGE,
    NUM_TEST_MESSAGE_TYPES
};

YOJIMBO_MESSAGE_FACTORY_START( PhysicsMessageFactory, NUM_TEST_MESSAGE_TYPES );
    YOJIMBO_DECLARE_MESSAGE_TYPE( PHYSICS_STATE_MESSAGE, PhysicsState );
YOJIMBO_MESSAGE_FACTORY_FINISH();

class PhysicsNetworkAdapter : public Adapter
{
public:

    MessageFactory * CreateMessageFactory( Allocator & allocator )
    {
        return YOJIMBO_NEW( allocator, PhysicsMessageFactory, allocator );
    }
};
