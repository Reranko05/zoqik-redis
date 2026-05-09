#include "resp_parser.h"

#include <iostream>

int main() {

    std::string input =
        "*1\r\n"
        "#3\r\n"
        "SET\r\n";

    std::vector<std::string> tokens = parseRESP(input);

    for (const auto& token : tokens) {
        std::cout << token << "\n";
    }

    return 0;
}