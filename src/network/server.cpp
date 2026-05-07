/*
    In this file we initialized a TCP socket using Winsock.

    socket() created a generic TCP socket with:
    1. AF_INET      -> IPv4
    2. SOCK_STREAM -> TCP
    3. IPPROTO_TCP -> TCP Protocol

    Then we configured the server address:
    - htons() converted the port number into network byte order (big-endian).
    - inet_pton() converted human-readable IP ("127.0.0.1") into binary form used internally by the OS.

    Then bind() assigned ownership of an IP + Port to this process.

    Then listen() transformed the generic TCP socket into a listening socket,
    allowing the OS to queue incoming TCP connections.

    accept() retrieved a client connection from the kernel accept queue
    and returned a new connection socket used for communication with that client.

    We used a while loop for continuous sequential client handling.
    Since this server is single-threaded and blocking,
    only one client can communicate at a time.

    recv() copied bytes from the kernel TCP receive buffer
    into the application buffer.

    TCP is stream-based, so one send() is NOT guaranteed to match one recv().
    Because of this, we used a persistent string buffer called incomingData
    to accumulate stream data across multiple recv() calls.

    Commands were delimited using '\n'.

    After appending received bytes into incomingData:
    - we searched for '\n'
    - extracted complete commands
    - erased processed commands from incomingData
    - preserved incomplete leftover bytes for future recv() calls

    This is called incremental stream parsing.

    After extracting a complete command:
    - the command was tokenized into structured arguments
    - CommandHandler validated and mapped commands to their respective database operations
    - Database provided persistent in-memory key-value storage

    The server used a single shared Database instance,
    allowing state persistence across multiple commands and clients.

    After command execution:
    - a response string was generated
    - '\n' was appended as a response delimiter
    - the response was sent back to the client

    send() is not guaranteed to send all bytes at once,
    so we used a send loop with:
    - totalSent tracking
    - remaining byte calculation
    - buffer offset shifting

    to ensure the complete response was transmitted reliably.

    Finally:
    - client sockets were cleaned up using closesocket()
    - server socket was closed
    - Winsock was cleaned up using WSACleanup()
*/

#include "server.h"
#include "../protocol/command_parser.h"
#include "../core/command_handler.h"
#include "../storage/database.h"

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

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

    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr); // converts readable IP to binary format used internally by OS

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

    std::cout << "Server Listening on 127.0.0.1:6379\n";

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

            size_t newlinePos;
            while ((newlinePos = incomingData.find('\n')) != std::string::npos) {  //  find newline, if newline exists i.e. not equal to npos (no posititon)

                std::string command = incomingData.substr(0, newlinePos);  // bytes before newline are one command

                incomingData.erase(0, newlinePos + 1);  // keep unprocessed leftover bytes that were after newline

                std::cout << "Command > " << command << '\n';
                std::vector<std::string> tokens = tokenizeCommand(command);

                std::string output = commandHandler.execute(tokens, database);
                std::cout << "Output > " << output << '\n';
                std::string output_full = output + '\n';
                int output_len = output_full.length();

                int totalSent = 0;

                while(totalSent < output_len){
                    int sent = send(
                        clientSocket,
                        output_full.c_str() + totalSent,
                        output_len - totalSent,
                        0
                    );

                    if (sent == SOCKET_ERROR) {
                        std::cout << "Sent Failed\n";
                        break;
                    }

                    totalSent += sent;
                }
            }

        }

        // Client Cleanup

        closesocket(clientSocket);
    }

    // Socket Cleanup

    closesocket(serverSocket);

    WSACleanup();

}