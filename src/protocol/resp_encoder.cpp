/*
    resp_encoder.cpp

    This file contains low-level RESP wire-format
    encoding helpers.

    --------------------------------------------------
    RESPONSIBILITY
    --------------------------------------------------

    This module converts application response
    data into valid RESP protocol byte sequences.

    Each function constructs a specific RESP
    response type according to the Redis
    Serialization Protocol specification.

    --------------------------------------------------
    SUPPORTED RESP TYPES
    --------------------------------------------------

    encodeSimpleString():
        generates RESP simple string frames

        Format:
            +<message>\r\n

    encodeBulkString():
        generates RESP bulk string frames

        Format:
            $<length>\r\n
            <payload>\r\n

    encodeError():
        generates RESP error frames

        Format:
            -<error_message>\r\n

    encodeNil():
        generates RESP nil bulk string responses

        Format:
            $-1\r\n

    --------------------------------------------------
    ENCODING FLOW
    --------------------------------------------------

    Bulk string encoding:
    - calculates payload length
    - converts length into ASCII form
    - constructs RESP-compliant payload framing

    All responses:
    - append CRLF terminators
    - generate transport-ready RESP byte streams

    --------------------------------------------------
    ARCHITECTURE ROLE
    --------------------------------------------------

    This module handles:
    - raw RESP byte construction

    but does NOT decide:
    - which RESP response type should be used

    High-level response selection logic is handled by:
        encodeResponse()

    refer to:
        resp_response_encoder.cpp
*/

#include "resp_encoder.h"

std::string encodeSimpleString(const std::string& input) {
    std::string encoded = "+" + input + "\r\n";
    return encoded;
}

std::string encodeBulkString(const std::string& input) {
    std::string length = std::to_string(input.size());
    std::string encoded = "$" + length + "\r\n" + input + "\r\n";
    return encoded;
}

std::string encodeError(const std::string& input) {
    std::string encoded = "-" + input + "\r\n";
    return encoded;
}

std::string encodeNil() {
    std::string encoded = "$-1\r\n";
    return encoded;
}