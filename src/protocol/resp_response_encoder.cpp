/*
    resp_response_encoder.cpp

    This file contains high-level RESP response
    encoding orchestration logic.

    --------------------------------------------------
    RESPONSIBILITY
    --------------------------------------------------

    encodeResponse() determines which RESP
    response type should be generated based on:
    - command type
    - command execution result
    - error state

    This module acts as a protocol-level
    response policy layer between:
    - application command execution
    - low-level RESP wire-format encoding

    --------------------------------------------------
    RESPONSE ENCODING FLOW
    --------------------------------------------------

    ECHO:
        returns RESP bulk string responses
        containing the provided payload

    GET:
        returns:
        - RESP bulk strings for valid values
        - RESP nil responses for missing keys
        - RESP error frames for failures

    Non-GET successful commands:
        return RESP simple string responses

    Error outputs:
        return RESP error frames

    --------------------------------------------------
    LOW-LEVEL ENCODERS
    --------------------------------------------------

    Actual RESP wire-format construction is
    delegated to:
    - encodeSimpleString()
    - encodeBulkString()
    - encodeError()
    - encodeNil()

    refer to:
        resp_encoder.cpp

    --------------------------------------------------
    ARCHITECTURE ROLE
    --------------------------------------------------

    This module separates:
    - protocol response policy decisions

    from:
    - raw RESP byte encoding logic

    allowing server orchestration code
    to remain independent from RESP
    response selection rules.
*/

#include "resp_response_encoder.h"
#include "resp_encoder.h"

std::string encodeResponse(const std::string& command, std::string& output, std::vector<std::string> tokens) {
    std::string encoded_output;

    if (command == "ECHO") {
        encoded_output = encodeBulkString(tokens[1]);
    }

    else if (command != "GET" && output.rfind("[ERROR]",0) == 0) {
        encoded_output = encodeError(output);
    }

    else if (command == "GET") {
        if (output == "[ERROR] key not found") {
            encoded_output = encodeNil();
        }

        else if (output.rfind("[ERROR]",0) == 0) {
            encoded_output = encodeError(output);
        }

        else {
            encoded_output = encodeBulkString(output);
        }

    }

    else {
        encoded_output = encodeSimpleString(output);
    }

    return encoded_output;
}