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

    While sending data back:
    - send() may send only partial bytes
    - so we tracked progress using totalSent
    - buffer + totalSent ensured already-sent bytes were not resent
    - bytesReceived - totalSent represented remaining unsent bytes

    Finally:
    - client sockets were cleaned up using closesocket()
    - server socket was closed
    - Winsock was cleaned up using WSACleanup()
*/

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")


int main() {

    // 1. Initialise Winsock

    WSADATA wsaData;

    int wsaResult = WSAStartup(MAKEWORD(2,2), &wsaData);

    if(wsaResult != 0) {
        std::cout << "WSAStartup Failed\n";
        return 1;
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
        return 1;
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

        return 1;
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
        return 1;
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

            return 1;
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
            }

            // 8. Send bytes back to client (Echo Server)

            int totalSent = 0; // initialize total bytes sent

            while(totalSent < bytesReceived) {  // run while total bytes sent is less than bytes recieved
                
                int sent = send(  // copies bytes from your application buffer to kernel TCP buffer
                    clientSocket,
                    buffer + totalSent,  // so that the bytes already sent are not sent again
                    bytesReceived - totalSent,  // because remaining bytes decrease as send
                    0
                );
                
                totalSent += sent;  // increasing the totalSent as we send bytes

                if (sent == SOCKET_ERROR) {  // Error handling if send fails
                    std::cout << "Sent Failed\n";
                    break;
                }
            }
        }

        // Client Cleanup

        closesocket(clientSocket);
    }

    // Socket Cleanup

    closesocket(serverSocket);

    WSACleanup();
    return 0;

}