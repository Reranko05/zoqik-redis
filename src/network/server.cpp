/*
    In this file we initialized a TCP socket using Winsock.

    socket() created a generic TCP socket with:
    1. AF_INET      -> IPv4
    2. SOCK_STREAM  -> TCP
    3. IPPROTO_TCP  -> TCP Protocol

    Then we configured the server address:
    - htons() converted the port number into network byte order (big-endian).
    - inet_pton() converted the human-readable IP address into
      the binary representation used internally by the OS.

    The server binds to:
    0.0.0.0:6379

    allowing external clients such as redis-cli
    and WSL environments to connect.

    bind() assigned ownership of the IP + Port
    combination to this process.

    listen() transformed the generic TCP socket
    into a listening socket, allowing the kernel
    to queue incoming TCP connections.

    accept() retrieved a client connection from
    the kernel accept queue and returned a new
    connection socket used for communication
    with that specific client.

    The server uses:
    - blocking I/O
    - sequential client handling
    - a single-threaded event loop

    so only one client can communicate with
    the server at a time.

    recv() copied bytes from the kernel TCP
    receive buffer into application memory.

    TCP is stream-oriented, meaning:
    one send() is NOT guaranteed to match one recv().

    Because of this, we used a persistent
    stream buffer called incomingData to
    accumulate bytes across multiple recv() calls.

    incomingData stores:
    - fragmented partial RESP frames
    - complete RESP frames
    - multiple RESP frames arriving together

    until they are fully processed.

    The server implements incremental RESP
    stream parsing.

    RESP requests begin with:
    *<argument_count>

    Each RESP argument consists of:
    - a bulk string length line
    - an argument payload line

    Example:

    *3
    $3
    SET
    $1
    a
    $1
    1

    After receiving stream bytes:
    - the RESP array header is parsed
    - expected RESP line count is calculated
    - the stream buffer is scanned until a complete RESP frame exists

    Incomplete RESP frames remain buffered
    inside incomingData until future recv()
    calls provide the remaining bytes.

    This preserves TCP stream correctness
    under packet fragmentation.

    Once a complete RESP frame is detected:
    - exact consumed bytes are extracted
    - consumed bytes are erased from incomingData
    - leftover unprocessed bytes remain buffered

    This is incremental framed stream parsing.

    Complete RESP frames are passed into:
    parseRESP()

    which validates RESP structure and converts
    the protocol frame into:
    std::vector<std::string> tokens

    representing structured command arguments.

    CommandHandler:
    - validates commands
    - maps commands to database operations
    - executes application logic

    Database provides:
    - in-memory key-value storage
    - TTL expiration
    - LRU eviction

    using:
    - unordered_map lookups
    - doubly linked list based LRU tracking

    The server maintains a single shared
    Database instance, allowing state persistence
    across multiple commands and client sessions.

    After command execution:
    - command results are converted into
      RESP wire-format responses

    using:
    - encodeSimpleString()
    - encodeBulkString()
    - encodeError()
    - encodeNil()

    GET requests return:
    - bulk strings for valid values
    - nil responses for missing keys

    Non-GET successful commands return:
    RESP simple strings.

    Errors return:
    RESP error frames.

    send() is not guaranteed to transmit
    all bytes at once.

    Because of this, a send loop is used with:
    - totalSent tracking
    - remaining byte calculation
    - pointer offset shifting

    to ensure complete response transmission.

    Finally:
    - client sockets are cleaned up using closesocket()
    - server socket is closed
    - Winsock resources are released using WSACleanup()
*/

#include "server.h"
#include "../protocol/command_parser.h"
#include "../protocol/resp_parser.h"
#include "../protocol/resp_encoder.h"
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

            while (true) {

                int args = 0;
                int remaining_lines;
                int byte_count = 0;
                if (incomingData.empty()) {
                    break;
                }
                if (incomingData[0] == '*') {
                    int i = 0;
                    while (i < incomingData.size() && incomingData[i] != '\r') {
                        i++;
                    }
                    if (i == incomingData.size()) {
                        std::cout << "[ERROR] end not found";
                        continue;
                    }
                    if (incomingData[i] == '\r') {
                        args = std::stoi(incomingData.substr(1,i-1));
                    }
                }

                remaining_lines = 1 + args*2;

                while (remaining_lines > 0 && byte_count < incomingData.size()) {
                    if (incomingData[byte_count] == '\n') {
                        remaining_lines--;
                    }
                    byte_count++;
                }
                
                if (remaining_lines != 0) break;
                

                std::string full_input(incomingData.substr(0,byte_count));
                incomingData.erase(0,byte_count);
                std::vector<std::string> tokens = parseRESP(full_input);

                if (tokens.empty()) {
                    std::cout << "[ERROR] no command found";
                    break;
                }

                std::string output = commandHandler.execute(tokens, database);
                std::cout << "Output > " << output << "\r\n";

                std::string command = tokens[0];

                std::transform(command.begin(), command.end(), command.begin(),
                                [](unsigned char c){return std::toupper(c); });
                
                std::string encoded_output;


                if (command != "GET" && output.rfind("[ERROR]",0) == 0) {
                        encoded_output = encodeError(output);
                }
                else if (command == "GET") {
                    if (output == "[ERROR] key not found") {
                        encoded_output = encodeNil();
                    }
                    else if (output.rfind("[ERROR]",0) == 0) {
                        encoded_output = encodeError(output);
                    }
                    else {
                        encoded_output = encodeBulkString(output);
                    }
                }
                else {
                    encoded_output = encodeSimpleString(output);
                }


                std::string output_full = encoded_output;
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