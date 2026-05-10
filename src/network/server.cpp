/*
    server.cpp

    This file contains the high-level TCP server orchestration layer.

    The server currently uses:
    - Winsock networking
    - blocking I/O
    - sequential client handling
    - a single-threaded event loop

    The server binds to:
    0.0.0.0:6379

    allowing external TCP clients such as:
    - redis-cli
    - WSL clients
    - custom TCP clients

    --------------------------------------------------
    NETWORKING FLOW
    --------------------------------------------------

    1. Initialize Winsock using WSAStartup()

    2. Create a TCP socket using:
       - AF_INET
       - SOCK_STREAM
       - IPPROTO_TCP

    3. Bind the socket to:
       0.0.0.0:6379

    4. Put the socket into listening mode using listen()

    5. Accept incoming client connections using accept()

    6. Continuously receive TCP stream bytes using recv()

    TCP is stream-oriented, meaning:
    recv() boundaries do NOT correspond to protocol frame boundaries.

    Because of this, the server maintains a persistent
    stream buffer called incomingData which accumulates
    bytes across multiple recv() calls.

    The server repeatedly:

    - receives TCP stream bytes
    - appends received bytes into incomingData
    - drains all complete RESP frames currently present
      inside the stream buffer
    - executes parsed commands
    - sends encoded RESP responses back to the client

    The server preserves:
    - incomplete fragmented frames
    - unprocessed leftover bytes
    - pipelined frames arriving together

    across recv() calls.

    --------------------------------------------------
    EXTERNAL MODULES
    --------------------------------------------------

    RESP stream framing and complete frame extraction:
        refer to resp_stream_parser.cpp

    RESP semantic parsing:
        refer to resp_parser.cpp

    RESP response encoding:
        refer to resp_response_encoder.cpp

    Reliable full-response TCP transmission:
        refer to socket_utils.cpp

    Command execution logic:
        refer to command_handler.cpp

    Database storage internals:
        refer to database.cpp

    --------------------------------------------------
    CLEANUP
    --------------------------------------------------

    Client sockets are cleaned up using:
        closesocket()

    Winsock resources are released using:
        WSACleanup()
*/

#include "server.h"
#include "socket_utils.h"
#include "../protocol/resp_stream_parser.h"
#include "../protocol/resp_response_encoder.h"
#include "../protocol/resp_parser.h"
#include "../core/command_handler.h"
#include "../storage/database.h"

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

void Server::start() {

    // 1. Initialise Winsock

    WSADATA wsaData;
    CommandHandler commandHandler;
    Database database;


    int wsaResult = WSAStartup(MAKEWORD(2,2), &wsaData);

    if(wsaResult != 0) {
        std::cout << "WSAStartup Failed\n";
        return;
    }

    std::cout << "Winsock Initialized\n";

    // 2. Create TCP Socket

    SOCKET serverSocket = socket(
        AF_INET, // IPv4
        SOCK_STREAM, // TCP, SOCK_DGRAM = UDP
        IPPROTO_TCP // TCP Protocol
    );

    if (serverSocket == INVALID_SOCKET) {
        std::cout << "Socket Creation Failed\n";
        WSACleanup();
        return;
    }

    std::cout << "Socket Created\n";
    
    // 3. Configure Server Address

    sockaddr_in serverAddress{}; // sockaddr_in stores IP + Port

    serverAddress.sin_family = AF_INET;

    serverAddress.sin_port = htons(6379); // hton stores byte in "big-endian byte order"

    inet_pton(AF_INET, "0.0.0.0", &serverAddress.sin_addr); // converts readable IP to binary format used internally by OS

    // 4. Bind Socket to IP + Port

    int bindResult = bind(   // Ownership claim "This process owns <IP:Port>"
        serverSocket,
        reinterpret_cast<sockaddr*>(&serverAddress),
        sizeof(serverAddress)
    );

    if (bindResult == SOCKET_ERROR) {
        std::cout << "Bind Failed\n";

        closesocket(serverSocket);
        WSACleanup();

        return;
    }

    std::cout << "Bind Successful\n";

    // 5. Put Socket into listening mode

    int listenResult = listen(   // Transforms generic TCP socket into listening socket
                                 // Now OS starts queueing incoming TCP connections
        serverSocket,
        SOMAXCONN  // maximum reasonable backlog queue
    );

    if (listenResult == SOCKET_ERROR) {
        std::cout << "Listen Failed\n";

        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    std::cout << "Server Listening on 0.0.0.0:6379\n";

    // 6. Connection Accept

    std::cout << "Waiting for client connection...\n";

    while (true) {

        SOCKET clientSocket = accept( // socket to communicate with clients
                                    // thread freezes and waits for client this is Blocking IO
            serverSocket,  // socket to accept future clients
            nullptr,
            nullptr
        );

        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Accept Failed\n";

            closesocket(serverSocket);
            WSACleanup();

            return;
        }

        std::cout << "Client Connected!!\n";

        // For continuous communication

        std::string incomingData;

        while (true) {

            char buffer[8]; // this is like temporary inbox, to store received bytes that TCP delivers
                            // fresh recieve buffer every iteration

            // 7. Copy Kernel TCP buffer (recv)

            int bytesReceived = recv(  // this copies available bytes from kernel TCP buffer into application memory
                clientSocket,
                buffer,
                sizeof(buffer) - 1, // -1 cuz we reserve one byte for null terminator -> '\0' 
                0
            );

            if (bytesReceived == 0) {
                std::cout << "Client Disconnected\n";
                break;
            }

            if (bytesReceived == SOCKET_ERROR) {
                std::cout << "recv Failed\n";
                break;
            }

            incomingData.append(buffer, bytesReceived);  // append exact received bytes into incomingData string
            std::string full_input;

            while (extractRESPFrame(incomingData, full_input)) {

                std::vector<std::string> tokens = parseRESP(full_input);

                if (tokens.empty()) {
                    std::cout << "[ERROR] no command found\n";
                    break;
                }

                std::string output = commandHandler.execute(tokens, database);
                std::cout << "Output > " << output << "\r\n";

                std::string command = tokens[0];

                std::transform(command.begin(), command.end(), command.begin(),
                                [](unsigned char c){return std::toupper(c); });
                
                
                std::string encoded_output = encodeResponse(command, output, tokens);

                sendAll(clientSocket, encoded_output);
            }

        }

        // Client Cleanup
        closesocket(clientSocket);
    }

    // Socket Cleanup

    closesocket(serverSocket);

    WSACleanup();

}