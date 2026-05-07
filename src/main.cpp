/*
    Entry point of the Redis-like server.

    Responsibilities:
    - bootstrap application
    - initialize server
    - start server lifecycle
*/

#include "network/server.h"

int main() {

    Server server;
    server.start();

}