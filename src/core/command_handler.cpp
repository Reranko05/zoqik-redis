/*
    CommandHandler maps parsed commands to their respective
    database operations or protocol responses.

    It validates whether the number of arguments matches
    the expected format for each command.

    Commands are normalized using std::toupper()
    so that:
    - set
    - SET
    - SeT

    are all treated as the same command.

    Example:
    "SET name A"

    maps to:
    database.set(key, value)

    GET command maps to database.get()
    and returns the stored value.

    DEL command maps to database.del()
    and removes the key-value pair from storage.

    PING command does not interact with the database.
    It is a protocol-level health check command
    that returns "PONG".

    If:
    - the command does not exist
    - the number of arguments is invalid

    then an error response is returned.
*/

#include "command_handler.h"

#include <algorithm>
#include <cctype>

std::string CommandHandler::execute(
    const std::vector<std::string>& tokens,
    Database& database
) {
    int args = tokens.size();
    
    if (args == 0) {
        return "No Command Found";
    }

    std::string command = tokens[0];

    std::transform(command.begin(), command.end(), command.begin(),
                    [](unsigned char c){return std::toupper(c); });
    
    if (command == "SET") {
        if (args == 3) {
            database.set(tokens[1],tokens[2]);
            return "Key Added Successfully";
        }
        else {
            return "[ERROR] wrong number of arguments for SET";
        }
    }

    else if (command == "GET") {
        if (args == 2) {
            std::string value = database.get(tokens[1]);
            return value;
        }
        else {
            return "[ERROR] wrong number of arguments for GET";
        }
    }

    else if (command == "DEL") {
        if (args == 2) {
            database.del(tokens[1]);
            return "Key Deleted Successfully";
        }
        else {
            return "[ERROR] wrong number of arguments for DEL";
        }
    }

    else if (command == "PING") {
        if (args == 1) {
            return "PONG";
        }
        else {
            return "[ERROR] wrong number of arguments for PING";
        }
    }

    else {
        return "[ERROR] command does not exist";
    }
}