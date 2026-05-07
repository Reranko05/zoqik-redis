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