#include "anet.h"
#include "stdio.h"
 
// ensure we are using winsock2 on windows.
#if (_WIN32_WINNT < 0x0601)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#define ENET_IMPLEMENTATION
#include "enet.h"

ENetAddress address = { 0 };
ENetPeer* server = { 0 };
ENetHost* client = {0};

void ANet::ConnectToServer(const char* ip, int port)
{
	enet_initialize();

	client = enet_host_create(NULL, 1, 1, 0, 0);

    

	enet_address_set_host(&address, ip);
	address.port = port;

	server = enet_host_connect(client, &address, 1, 0);
}

int8_t ReadByte(ENetPacket* packet, size_t* offset)
{
    // make sure we have not gone past the end of the data we were sent
    if (*offset > packet->dataLength)
        return 0;

    // cast the data to a byte so we can increment it in 1 byte chunks
    uint8_t* ptr = (uint8_t*)packet->data;

    // get the byte at the current offset
    uint8_t data = ptr[(*offset)];

    // move the offset over 1 byte for the next read
    *offset = *offset + 1;

    return data;
}

/// <summary>
/// Read a signed short from the network packet
/// Note that this assumes the packet is in the host's byte ordering
/// In reality read/write code should use ntohs and htons to convert from network byte order to host byte order, so both big endian and little endian machines can play together
/// </summary>
/// <param name="packet">The packet to read from<</param>
/// <param name="offset">A pointer to an offset that is updated, this should be passed to other read functions so they read from the correct place</param>
/// <returns>The signed short that is read</returns>
int16_t ReadShort(ENetPacket* packet, size_t* offset)
{
    // make sure we have not gone past the end of the data we were sent
    if (*offset > packet->dataLength)
        return 0;

    // cast the data to a byte at the offset
    uint8_t* data = (uint8_t*)packet->data;
    data += (*offset);

    // move the offset over 2 bytes for the next read
    *offset = (*offset) + 2;

    // cast the data pointer to a short and return a copy
    return *(int16_t*)data;
}

Vector2 ReadPosition(ENetPacket* packet, size_t* offset)
{
    Vector2 pos = { 0 };
    pos.x = ReadShort(packet, offset);
    pos.y = ReadShort(packet, offset);

    return pos;
}

// add your own functions/variables

// Data about players
typedef struct
{
    // true if the player is active and valid
    bool Active;

    // the last known location of the player on the field
    Vector2 Position;

    // the direction they were going
    Vector2 Direction;

    // the time we got the last update
    double UpdateTime;

    //where we think this item is right now based on the movement vector
    Vector2 ExtrapolatedPosition;


}RemotePlayer;

// All the different commands that can be sent over the network
typedef enum
{
    // Server -> Client, You have been accepted. Contains the id for the client player to use
    AcceptPlayer = 1,

    // Server -> Client, Add a new player to your simulation, contains the ID of the player and a position
    AddPlayer = 2,

    // Server -> Client, Remove a player from your simulation, contains the ID of the player to remove
    RemovePlayer = 3,

    // Server -> Client, Update a player's position in the simulation, contains the ID of the player and a position
    UpdatePlayer = 4,

    // Client -> Server, Provide an updated location for the client's player, contains the postion to update
    UpdateInput = 5,
}NetworkCommands;


// how long in seconds since the last time we sent an update
double LastInputSend = -100;

// how long to wait between updates (20 update ticks a second)
double InputUpdateInterval = 1.0f / 20.0f;

double LastNow = 0;

int LocalPlayerId = -1;

RemotePlayer Players[MAX_PLAYERS] = { 0 };

// A new remote player was added to our local simulation
void HandleAddPlayer(ENetPacket* packet, size_t* offset)
{
    // find out who the server is talking about
    int remotePlayer = ReadByte(packet, offset);
    if (remotePlayer >= MAX_PLAYERS || remotePlayer == LocalPlayerId)
        return;

    // set them as active and update the location
    Players[remotePlayer].Active = true;
    Players[remotePlayer].Position = ReadPosition(packet, offset);
    Players[remotePlayer].Direction = ReadPosition(packet, offset);
    Players[remotePlayer].UpdateTime = LastNow;

    // In a more robust game, this message would have more info about the new player, such as what sprite or model to use, player name, or other data a client would need
    // this is where static data about the player would be sent, and any initial state needed to setup the local simulation
}

// A remote player has left the game and needs to be removed from the local simulation
void HandleRemovePlayer(ENetPacket* packet, size_t* offset)
{
    // find out who the server is talking about
    int remotePlayer = ReadByte(packet, offset);
    if (remotePlayer >= MAX_PLAYERS || remotePlayer == LocalPlayerId)
        return;

    // remove the player from the simulation. No other data is needed except the player id
    Players[remotePlayer].Active = false;
}

// The server has a new position for a player in our local simulation
void HandleUpdatePlayer(ENetPacket* packet, size_t* offset)
{
    // find out who the server is talking about
    int remotePlayer = ReadByte(packet, offset);
    if (remotePlayer >= MAX_PLAYERS || remotePlayer == LocalPlayerId || !Players[remotePlayer].Active)
        return;

    // update the last known position and movement
    Players[remotePlayer].Position = ReadPosition(packet, offset);
    Players[remotePlayer].Direction = ReadPosition(packet, offset);
    Players[remotePlayer].UpdateTime = LastNow;

    // in a more robust game this message would have a tick ID for what time this information was valid, and extra info about
    // what the input state was so the local simulation could do prediction and smooth out the motion
}

// process one frame of updates
void ANet::Update(double now, float deltaT)
{
    LastNow = now;
    // if we are not connected to anything yet, we can't do anything, so bail out early
    if (server == NULL)
        return;

    // Check if we have been accepted, and if so, check the clock to see if it is time for us to send the updated position for the local player
    // we do this so that we don't spam the server with updates 60 times a second and waste bandwidth
    // in a real game we'd send our normalized movement vector or input keys along with what the current tick index was
    // this way the server can know how long it's been since the last update and can do interpolation to know were we are between updates.
    if (LocalPlayerId >= 0 && now - LastInputSend > InputUpdateInterval)
    {
        // Pack up a buffer with the data we want to send
        uint8_t buffer[9] = { 0 }; // 9 bytes for a 1 byte command number and two bytes for each X and Y value
        buffer[0] = (uint8_t)UpdateInput;   // this tells the server what kind of data to expect in this packet
        *(int16_t*)(buffer + 1) = (int16_t)Players[LocalPlayerId].Position.x;
        *(int16_t*)(buffer + 3) = (int16_t)Players[LocalPlayerId].Position.y;
        *(int16_t*)(buffer + 5) = (int16_t)Players[LocalPlayerId].Direction.x;
        *(int16_t*)(buffer + 7) = (int16_t)Players[LocalPlayerId].Direction.y;

        // copy this data into a packet provided by enet (TODO : add pack functions that write directly to the packet to avoid the copy)
        ENetPacket* packet = enet_packet_create(buffer, 9, ENET_PACKET_FLAG_RELIABLE);

        // send the packet to the server
        enet_peer_send(server, 0, packet);

        // NOTE enet_host_service will handle releasing send packets when the network system has finally sent them,
        // you don't have to destroy them

        // mark that now was the last time we sent an update
        LastInputSend = now;
    }

    // read one event from enet and process it
    ENetEvent Event = {};

    // Check to see if we even have any events to do. Since this is a a client, we don't set a timeout so that the client can keep going if there are no events
    if (enet_host_service(client, &Event, 0) > 0)
    {
        // see what kind of event it is
        switch (Event.type)
        {
            // the server sent us some data, we should process it
        case ENET_EVENT_TYPE_RECEIVE:
        {
            // we know that all valid packets have a size >= 1, so if we get this, something is bad and we ignore it.
            if (Event.packet->dataLength < 1)
                break;

            // keep an offset of what data we have read so far
            size_t offset = 0;

            // read off the command that the server wants us to do
            NetworkCommands command = (NetworkCommands)ReadByte(Event.packet, &offset);

            // if the server has not accepted us yet, we are limited in what packets we can receive
            if (LocalPlayerId == -1)
            {
                if (command == AcceptPlayer)    // this is the only thing we can do in this state, so ignore anything else
                {
                    // See who the server says we are
                    LocalPlayerId = ReadByte(Event.packet, &offset);

                    // Make sure that it makes sense
                    if (LocalPlayerId < 0 || LocalPlayerId > MAX_PLAYERS)
                    {
                        LocalPlayerId = -1;
                        break;
                    }

                    // Force the next frame to do an update by pretending it's been a very long time since our last update
                    LastInputSend = -InputUpdateInterval;

                    // We are active
                    Players[LocalPlayerId].Active = true;

                    // Set our player at some location on the field.
                    // optimally we would do a much more robust connection negotiation where we tell the server what our name is, what we look like
                    // and then the server tells us where we are
                    // But for this simple test, everyone starts at the same place on the field
                    Players[LocalPlayerId].Position = { 100, 100 };
                }
            }
            else // we have been accepted, so process play messages from the server
            {
                // see what the server wants us to do
                switch (command)
                {
                case AddPlayer:
                    HandleAddPlayer(Event.packet, &offset);
                    break;

                case RemovePlayer:
                    HandleRemovePlayer(Event.packet, &offset);
                    break;

                case UpdatePlayer:
                    HandleUpdatePlayer(Event.packet, &offset);
                    break;
                }
            }
            // tell enet that it can recycle the packet data
            enet_packet_destroy(Event.packet);
            break;
        }

        // we were disconnected, we have a sad
        case ENET_EVENT_TYPE_DISCONNECT:
            server = NULL;
            LocalPlayerId = -1;
            break;
        }
    }

    // update all the remote players with an interpolated position based on the last known good pos and how long it has been since an update
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (i == LocalPlayerId || !Players[i].Active)
            continue;
        double delta = LastNow - Players[i].UpdateTime;
        Players[i].ExtrapolatedPosition = Vector2Add(Players[i].Position, Vector2Scale(Players[i].Direction, delta));
    }
}

// force a disconnect by shutting down enet
void ANet::Disconnect()
{
    // close our connection to the server
    if (server != NULL)
        enet_peer_disconnect(server, 0);

    // close our client
    if (client != NULL)
        enet_host_destroy(client);

    client = NULL;
    server = NULL;

    // clean up enet
    enet_deinitialize();
}

// true if we are connected and have been accepted
bool ANet::Connected()
{
    if (server == NULL) {
        printf("SERVER NOT SET UP OR CONNECTING\N");
    }
    if (LocalPlayerId < 0) {
        printf("PLR NOT CONNECTED\N");
    }
    return server != NULL && LocalPlayerId >= 0;
}

int ANet::GetLocalPlayerId()
{
    return LocalPlayerId;
}

// add the input to our local position and make sure we are still inside the field
void ANet::UpdateLocalPlayer(Vector2* movementDelta, float deltaT)
{
    // if we are not accepted, we can't update
    if (LocalPlayerId < 0)
        return;

    // add the movement to our location
    Players[LocalPlayerId].Position = Vector2Add(Players[LocalPlayerId].Position, Vector2Scale(*movementDelta, deltaT));

    // make sure we are in bounds.
    // In a real game both the client and the server would do this to help prevent cheaters
    if (Players[LocalPlayerId].Position.x < 0)
        Players[LocalPlayerId].Position.x = 0;

    if (Players[LocalPlayerId].Position.y < 0)
        Players[LocalPlayerId].Position.y = 0;

    Players[LocalPlayerId].Direction = *movementDelta;
}

// get the info for a particular player
bool ANet::GetPlayerPos(int id, Vector2* pos)
{
    // make sure the player is valid and active
    if (id < 0 || id >= MAX_PLAYERS || !Players[id].Active)
        return false;

    // copy the location (real or extrapolated)
    if (id == LocalPlayerId)
        *pos = Players[id].Position;
    else
        *pos = Players[id].ExtrapolatedPosition;
    return true;
}