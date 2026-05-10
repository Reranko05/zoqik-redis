/*
    socket_utils.cpp

    This file contains low-level TCP socket utility helpers.

    --------------------------------------------------
    RESPONSIBILITY
    --------------------------------------------------

    sendAll() provides reliable full-response
    TCP transmission.

    TCP send() is NOT guaranteed to transmit
    all bytes in a single call.

    A single send() may:
    - partially transmit bytes
    - transmit fewer bytes than requested
    - require multiple send() calls to complete
      the full response transmission

    Because of this, sendAll():

    - repeatedly calls send()
    - tracks total transmitted bytes
    - calculates remaining unsent bytes
    - shifts the transmit pointer forward

    until the complete response buffer
    has been transmitted.

    --------------------------------------------------
    TRANSMISSION FLOW
    --------------------------------------------------

    totalSent:
        tracks cumulative transmitted bytes

    output_len:
        stores total response size

    send():
        transmits remaining unsent bytes
        from the current buffer offset

    Pointer offset shifting:

        output_full.c_str() + totalSent

    advances the transmit starting position
    to the first unsent byte.

    --------------------------------------------------
    ERROR HANDLING
    --------------------------------------------------

    If send() returns SOCKET_ERROR:
    - transmission failure is reported
    - transmission loop terminates

    --------------------------------------------------
    USAGE
    --------------------------------------------------

    This module is used by:
        server.cpp

    allowing server orchestration logic
    to remain independent from low-level
    TCP partial-send handling.
*/

#include "socket_utils.h"

#include <iostream>

bool sendAll(SOCKET clientSocket, const std::string& output_full) {
    int totalSent = 0;
    int output_len = output_full.length();

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
    return true;
}