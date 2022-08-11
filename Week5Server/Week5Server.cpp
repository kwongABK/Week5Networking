#include <enet/enet.h>
#include "Message.h"

#include <iostream>
#include <string>
#include <chrono>
using namespace std;

ENetAddress address;
ENetHost* server = nullptr;
string username;
int secretNumber = 0;
bool IsRunning = true;

bool CreateServer()
{
    /* Bind the server to the default localhost.     */
    /* A specific host address can be specified by   */
    /* enet_address_set_host (& address, "x.x.x.x"); */
    address.host = ENET_HOST_ANY;
    /* Bind the server to port 1234. */
    address.port = 1234;
    server = enet_host_create(&address /* the address to bind the server host to */,
        32      /* allow up to 32 clients and/or outgoing connections */,
        2      /* allow up to 2 channels to be used, 0 and 1 */,
        0      /* assume any amount of incoming bandwidth */,
        0      /* assume any amount of outgoing bandwidth */);

    return server != nullptr;
}

void SendPacket(string message)
{
    Message* pack = new Message();
    pack->username = username;
    pack->content = message;
    ENetPacket* packet = enet_packet_create(pack,
        sizeof(Message) + 1,
        ENET_PACKET_FLAG_RELIABLE);
    cout << username << ": " << message << endl;

    /* Send the packet to the peer over channel id 0. */
    /* One could also broadcast the packet by         */
    enet_host_broadcast(server, 0, packet);
    enet_host_flush(server);
}

void HandlePacket(ENetEvent event)
{
    Message* received = (Message*)(event.packet->data);
    string sender = received->username;
    string message = received->content;
    cout << sender << ": " << message << endl;

    if (received->content == "quit")
    {
        IsRunning = false;
    }
    else
    {
        int guess = atoi(&message[0]);
        if (guess < secretNumber)
        {
            SendPacket("Too low.");
        }
        else if (guess > secretNumber)
        {
            SendPacket("Too high.");
        }
        else
        {
            SendPacket("Correct!");
            secretNumber = (rand() % 100) + 1;
            cout << "Your new secret number is " << secretNumber << endl;
            SendPacket("New # chosen.");
        }
    }

    /* Clean up the packet now that we're done using it. */
    enet_packet_destroy(event.packet);
}

int main(int argc, char** argv)
{
    cout << "Enter a name for the server:" << endl;
    cin >> username;

    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        cout << "An error occurred while initializing ENet." << endl;
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    if (!CreateServer())
    {
        fprintf(stderr,
            "An error occurred while trying to create an ENet server host.\n");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));
    secretNumber = (rand() % 100) + 1;
    cout << "Hello, " << username << endl;
    cout << "Your secret number is " << secretNumber << endl;

    while (IsRunning)
    {
        ENetEvent event;
        /* Wait up to 1000 milliseconds for an event. */
        while (enet_host_service(server, &event, 1000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                cout << "A new client connected from "
                    << event.peer->address.host
                    << ":" << event.peer->address.port
                    << endl;
                /* Store any relevant client information here. */
                event.peer->data = (void*)("Client information");

                SendPacket("Guess my #!");
                SendPacket("Range is 1-100");
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                HandlePacket(event);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                cout << (char*)event.peer->data << "disconnected." << endl;
                /* Reset the peer's client information. */
                event.peer->data = NULL;
            }
        }
    }

    if (server != nullptr)
    {
        enet_host_destroy(server);
    }

    return EXIT_SUCCESS;
}