/*
    resp_parser.cpp

    This file contains RESP semantic parsing logic.

    --------------------------------------------------
    RESPONSIBILITY
    --------------------------------------------------

    parseRESP() validates RESP request structure
    and converts a complete RESP frame into:

        std::vector<std::string>

    representing structured command arguments.

    This module operates only on:
    - complete RESP frames

    and does NOT handle:
    - TCP stream buffering
    - fragmented frame assembly
    - frame boundary extraction

    refer to:
        resp_stream_parser.cpp

    --------------------------------------------------
    RESP PARSING FLOW
    --------------------------------------------------

    1. The RESP frame is split into
       individual protocol lines

    2. CRLF terminators are removed

    3. The RESP array header:

        *<argument_count>

       is parsed to determine expected
       argument count

    4. Each bulk string entry:

        $<length>

       is validated and parsed

    5. Bulk string payloads are extracted
       into command argument tokens

    --------------------------------------------------
    VALIDATION
    --------------------------------------------------

    The parser validates:
    - RESP array header existence
    - bulk string argument structure
    - argument count consistency
    - payload length correctness
    - sufficient argument availability

    Invalid RESP structures return:
        {}

    indicating semantic parsing failure.

    --------------------------------------------------
    OUTPUT
    --------------------------------------------------

    Successful parsing produces:

        {"SET", "a", "1"}

    style command token vectors which are
    later passed into command execution logic.

    --------------------------------------------------
    EXTERNAL MODULES
    --------------------------------------------------

    Parsed command tokens are later processed by:
        CommandHandler

    refer to:
        command_handler.cpp
*/

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