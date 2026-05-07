#pragma once 

#include "../storage/database.h"

#include <string>
#include <vector>

class CommandHandler {
public:
    std::string execute(
        const std::vector<std::string>& tokens,
        Database& database
    );
};