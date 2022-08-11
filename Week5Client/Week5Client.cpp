#include <enet/enet.h>
#include "Message.h"

#include <iostream>
#include <thread>
using namespace std;

ENetAddress address;
ENetHost* client = nullptr;
string username;
bool IsRunning = true;

bool CreateClient()
{
    client = enet_host_create(NULL /* create a client host */,
        1 /* only allow 1 outgoing connection */,
        2 /* allow up 2 channels to be used, 0 and 1 */,
        0 /* assume any amount of incoming bandwidth */,
        0 /* assume any amount of outgoing bandwidth */);

    return client != nullptr;
}

void SendPacket(string message)
{
    Message* pack = new Message();
    pack->username = username;
    pack->content = message;
    ENetPacket* packet = enet_packet_create(pack,
        sizeof(Message) + 1,
        ENET_PACKET_FLAG_RELIABLE);

    if (message == "quit")
    {
        IsRunning = false;
    }

    /* Send the packet to the peer over channel id 0. */
    /* One could also broadcast the packet by         */
    enet_host_broadcast(client, 0, packet);
    enet_host_flush(client);
}

void HandlePacket(ENetEvent event)
{
    Message* received = (Message*)(event.packet->data);
    cout << received->username << ": " << received->content
        << endl;
    /* Clean up the packet now that we're done using it. */
    enet_packet_destroy(event.packet);
}

void PacketListener()
{
    while (IsRunning)
    {
        ENetEvent event;
        /* Wait up to 1000 milliseconds for an event. */
        while (enet_host_service(client, &event, 1000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                HandlePacket(event);
            }
        }
    }
}

int main(int argc, char** argv)
{
    cout << "Welcome to IntGuessr" << endl;
    cout << "Please enter your name." << endl;
    cin >> username;

    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        cout << "An error occurred while initializing ENet." << endl;
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);
    
    if (!CreateClient())
    {
        fprintf(stderr,
            "An error occurred while trying to create an ENet client host.\n");
        exit(EXIT_FAILURE);
    }

    ENetAddress address;
    ENetEvent event;
    ENetPeer* peer;
    /* Connect to some.server.net:1234. */
    enet_address_set_host(&address, "127.0.0.1");
    address.port = 1234;
    /* Initiate the connection, allocating the two channels 0 and 1. */
    peer = enet_host_connect(client, &address, 2, 0);
    if (peer == NULL)
    {
        fprintf(stderr,
            "No available peers for initiating an ENet connection.\n");
        exit(EXIT_FAILURE);
    }
    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service(client, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        cout << "Connection to 127.0.0.1:1234 succeeded." << endl;
    }
    else
    {
        /* Either the 5 seconds are up or a disconnect event was */
        /* received. Reset the peer in the event the 5 seconds   */
        /* had run out without any significant event.            */
        enet_peer_reset(peer);
        cout << "Connection to 127.0.0.1:1234 failed." << endl;
    }

    thread Listener(PacketListener);
    while (IsRunning)
    {
        // Read new message.
        string message;
        cin >> message;

        // Delete the input line and replace it with "USER: MESSAGE"
        cout << "\x1b[2K"; // Delete current line
        // i=1 because we included the first line
        cout
            << "\x1b[1A" // Move cursor up one
            << "\x1b[2K"; // Delete the entire line
        cout << "\r"; // Resume the cursor at beginning of line
        cout << username << ": " << message << endl;

        SendPacket(message);
    }

    Listener.join();
    if (client != nullptr)
    {
        enet_host_destroy(client);
    }


    return EXIT_SUCCESS;
}