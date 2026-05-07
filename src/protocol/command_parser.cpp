/*
    This file is responsible for tokenizing parsed command strings.

    Input:
    - A complete command string extracted from the TCP stream parser.

    Example:
    "SET name Aaditya"

    Output:
    ["SET", "name", "Aaditya"]

    This layer exists because applications work with structured commands,
    not raw strings or TCP byte streams.

    We used std::stringstream with space-delimited parsing
    to split commands into individual tokens.

    This file does NOT handle:
    - sockets
    - recv/send
    - TCP stream parsing
    - command execution
    - database state

    Its only responsibility is:
    command string -> structured token vector

    Current limitation:
    - values containing spaces are split into multiple tokens
    - this is acceptable for now and will later be solved properly using RESP
*/

#include "command_parser.h"

#include <sstream>

std::vector<std::string> tokenizeCommand(const std::string& command) {
    
    std::vector<std::string> tokens;
    std::string token;
    std::stringstream tokenStream(command);
    char delim = ' ';

    while(std::getline(tokenStream, token, delim)) {
        tokens.push_back(token);
    }

    return tokens;
}