/*
    CommandHandler maps parsed commands to their respective
    database operations.

    It validates whether the number of arguments matches
    the expected format for each command.

    Example:
    "SET name A"

    maps to:
    database.set(key, value)

    GET command maps to database.get()
    and returns the stored value.

    DEL command maps to database.del()
    and removes the key-value pair from storage.

    If the command is invalid or unsupported,
    an error message is returned.
*/

#include "command_handler.h"

std::string CommandHandler::execute(
    const std::vector<std::string>& tokens,
    Database& database
) {
    int args = tokens.size();
    
    if (args == 0) {
        return "No Command Found";
    }

    std::string command = tokens[0];
    
    if (command == "SET") {
        if (args == 3) {
            database.set(tokens[1],tokens[2]);
            return "Key Added Successfully";
        }
        else {
            return "Invalid Number of Args";
        }
    }

    else if (command == "GET") {
        if (args == 2) {
            std::string value = database.get(tokens[1]);
            return value;
        }
        else {
            return "Invalid Number of Args";
        }
    }

    else if (command == "DEL") {
        if (args == 2) {
            database.del(tokens[1]);
            return "Key Deleted Successfully";
        }
        else {
            return "Invalid Number of Args";
        }
    }

    else {
        return "Invalid Command";
    }
}