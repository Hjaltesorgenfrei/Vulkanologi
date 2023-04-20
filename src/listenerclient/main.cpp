/*
    Yojimbo Client Example (insecure)

    Copyright © 2016 - 2019, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <yojimbo.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <signal.h>
#include <glm/glm.hpp>
#include "SharedServerSettings.hpp"

using namespace yojimbo;

static volatile int quit = 0;

void interrupt_handler( int /*dummy*/ )
{
    quit = 1;
}

void printNetworkInfo(Client& client) 
{
    NetworkInfo info;
    client.GetNetworkInfo(info);
    printf("client: sent: %lld, received: %lld, acked: %lld, lost: %f, rtt: %f ms\n", info.numPacketsSent, info.numPacketsReceived, info.numPacketsAcked, info.packetLoss, info.RTT);
}

int ClientMain( int argc, char * argv[] )
{   
    printf( "\nconnecting client (insecure)\n" );

    double time = 100.0;

    uint64_t clientId = 0;
    random_bytes( (uint8_t*) &clientId, 8 );
    printf( "client id is %.16" PRIx64 "\n", clientId );
    PhysicsNetworkAdapter adapter;
    ClientServerConfig config;
    config.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    
    Client client( GetDefaultAllocator(), Address("0.0.0.0"), config, adapter, time );

    Address serverAddress( "127.0.0.1", ServerPort );

    if ( argc == 2 )
    {
        Address commandLineAddress( argv[1] );
        if ( commandLineAddress.IsValid() )
        {
            if ( commandLineAddress.GetPort() == 0 )
                commandLineAddress.SetPort( ServerPort );
            serverAddress = commandLineAddress;
        }
    }

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    client.InsecureConnect( privateKey, clientId, serverAddress );

    char addressString[256];
    client.GetAddress().ToString( addressString, sizeof( addressString ) );
    printf( "client address is %s\n", addressString );

    const double deltaTime = 0.01f;

    signal( SIGINT, interrupt_handler );

    while ( !quit)
    {
        client.SendPackets();

        client.ReceivePackets();

        if ( client.IsDisconnected() )
            break;

        while (auto message = client.ReceiveMessage(0)) {
            if (message->GetType() == PHYSICS_STATE_MESSAGE) {
                auto physicsState = (PhysicsState*)message;
                printf("tick: %d, entities: %d\n", physicsState->tick, physicsState->entities);
                const int blockSize = physicsState->GetBlockSize();
                const uint8_t * blockData = physicsState->GetBlockData();
                for (uint32_t i = 0; i < physicsState->entities; i++) {
                    glm::vec3 position;
                    memcpy(&position, blockData, sizeof(glm::vec3));
                    blockData += sizeof(glm::vec3);
                    printf("entity %d: position: (%f, %f, %f)\n", i, position.x, position.y, position.z);
                }
            }
            client.ReleaseMessage(message);
        }
     
        time += deltaTime;

        client.AdvanceTime( time );

        if ( client.ConnectionFailed() )
            break;

        printNetworkInfo(client);

        yojimbo_sleep( deltaTime );
    }

    client.Disconnect();

    return 0;
}

int main( int argc, char * argv[] )
{
    if ( !InitializeYojimbo() )
    {
        printf( "error: failed to initialize Yojimbo!\n" );
        return 1;
    }

    yojimbo_log_level( YOJIMBO_LOG_LEVEL_INFO );

    srand( (unsigned int) time( NULL ) );

    int result = ClientMain( argc, argv );

    ShutdownYojimbo();

    printf( "\n" );

    return result;
}