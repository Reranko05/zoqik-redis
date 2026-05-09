#include "resp_parser.h"

#include <iostream>

std::vector<std::string> parseRESP(
    const std::string& input
) {
    std::vector<std::string> res;
    std::vector<std::string> tokens;
    std::string token;

    for (char c : input) {
        if (c == '\n') {
            res.push_back(token);
            token = "";
        }
        else {
            if (c != '\r') {
                token += c;
            }
        }   
    }

    if (!token.empty()) {
        res.push_back(token);
    }

    int args = 0;

    if (res[0][0] == '*') {
        args = std::stoi(res[0].substr(1));
    }

    for (int i = 1; i < res.size(); i += 2) {
        int str_len;
        if (res[i][0] == '$') {
            str_len = std::stoi(res[i].substr(1));
        }
        else {
            std::cout << "[ERROR] argument should start with '$'";
            return {};
        }

        if (i + 1 >= res.size()) {
            std::cout << "[ERROR] not enough args";
            return {};
        }

        std::string act_str = res[i+1];

        if (act_str.size() == str_len) {
            tokens.push_back(act_str);
        }
    }

    if (tokens.size() == args) {
        return tokens;
    }
    else {
        std::cout << "[ERROR] no. of args mismatch";
        return {};
    }
}