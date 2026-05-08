/*
    CommandHandler maps parsed commands to their respective
    database operations or protocol-level responses.

    It validates:
    - command syntax
    - number of arguments
    - TTL values for expiring keys

    Commands are normalized using std::toupper()
    so that:
    - set
    - SET
    - SeT

    are all treated as the same command.

    Supported Commands:

    1. Persistent SET
       Example:
       "SET name Aaditya"

       maps to:
       database.p_set(key, value)

       This stores the key permanently
       without expiration metadata.

    2. Expiring SET
       Example:
       "SET name Aaditya EX 10"

       maps to:
       database.set(key, value, ttl)

       This stores the key with a TTL
       (Time To Live) in seconds.

    3. GET
       maps to:
       database.get()

       Returns:
       - stored value if key exists
       - error if key does not exist
       - triggers lazy expiration cleanup
         for expired keys

    4. DEL
       maps to:
       database.del()

       Removes:
       - key-value pair
       - expiration metadata if present

    5. PING
       Protocol-level health check command.

       Returns:
       "PONG"

       without interacting with database storage.

    TTL Handling:
    - TTL values are parsed using std::stoi()
    - invalid or negative TTL values return errors
    - exceptions from invalid numeric parsing
      are safely handled using try/catch

    If:
    - command does not exist
    - syntax is invalid
    - argument count is incorrect
    - TTL value is invalid

    then an error response is returned.
*/

#include "command_handler.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

std::string CommandHandler::execute(
    const std::vector<std::string>& tokens,
    Database& database
) {
    int args = tokens.size();
    
    if (args == 0) {
        return "[ERROR] No Command Found";
    }

    std::string command = tokens[0];

    std::transform(command.begin(), command.end(), command.begin(),
                    [](unsigned char c){return std::toupper(c); });
    
    if (command == "SET") {
        if (args == 3) {
            database.p_set(tokens[1], tokens[2]);
            return "Key Added Successfully";
        }
        else if (args == 5) {

            std::string ttlCommand = tokens[3];
            std::transform(ttlCommand.begin(), ttlCommand.end(), ttlCommand.begin(),
                            [](unsigned char c){return std::toupper(c); });

            if (ttlCommand == "EX") {
                try {
                    int ttl_val = std::stoi(tokens[4]);
                    if (ttl_val > 0) {
                        database.set(tokens[1], tokens[2], ttl_val);
                        return "Key Added Successfully";
                    }
                    else {
                        return "[ERROR] invalid ttl value";
                    }
                }
                catch (const std::invalid_argument& e) {
                    return "[ERROR] invalid ttl value";
                }
            }
            else {
                return "[ERROR] command not found";
            }
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