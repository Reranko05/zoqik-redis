/*
    resp_stream_parser.cpp

    This file contains incremental RESP stream
    framing and complete frame extraction logic.

    --------------------------------------------------
    RESPONSIBILITY
    --------------------------------------------------

    extractRESPFrame() determines whether the
    current TCP stream buffer contains one
    complete RESP frame.

    If a complete frame exists:
    - exact frame bytes are extracted
    - consumed bytes are removed
    - leftover stream bytes are preserved

    If the frame is incomplete:
    - stream state remains buffered
    - the caller must recv() additional bytes

    --------------------------------------------------
    TCP STREAM MODEL
    --------------------------------------------------

    TCP is stream-oriented, meaning:
    recv() boundaries do NOT correspond to
    RESP frame boundaries.

    Because of this, incomingData may contain:
    - fragmented partial RESP frames
    - one complete RESP frame
    - multiple RESP frames together
    - partial + complete frame mixtures

    This module incrementally scans the stream
    buffer until a complete RESP frame boundary
    can be determined safely.

    --------------------------------------------------
    RESP FRAME STRUCTURE
    --------------------------------------------------

    RESP requests begin with:

        *<argument_count>

    Each RESP argument consists of:
    - a bulk string length line
    - an argument payload line

    Example:

        *3
        $3
        SET
        $1
        a
        $1
        1

    --------------------------------------------------
    FRAME EXTRACTION FLOW
    --------------------------------------------------

    1. Leading CRLF bytes are discarded

    2. The RESP array header is parsed
       to determine argument count

    3. Each bulk string header is scanned
       to determine payload length

    4. Exact byte boundaries for the
       complete RESP frame are calculated

    5. If the frame is incomplete:
       extraction stops safely and
       the caller must recv() more data

    6. If the frame is complete:
       - exact frame bytes are extracted
       - consumed bytes are erased
       - leftover stream bytes remain buffered

    --------------------------------------------------
    PIPELINING SUPPORT
    --------------------------------------------------

    This module supports:
    - fragmented TCP delivery
    - multiple RESP frames in one recv()
    - incremental frame draining
    - basic RESP pipelining

    by preserving unprocessed bytes
    inside incomingData after each
    successful frame extraction.

    --------------------------------------------------
    RETURN SEMANTICS
    --------------------------------------------------

    return true:
        one complete RESP frame was extracted

    return false:
        no complete RESP frame currently exists
        inside the stream buffer

    --------------------------------------------------
    EXTERNAL MODULES
    --------------------------------------------------

    Extracted RESP frames are later passed into:
        parseRESP()

    refer to:
        resp_parser.cpp
*/

#include "resp_stream_parser.h"

#include <iostream>

bool extractRESPFrame(std::string& incomingData, std::string& full_input) {
    int args = 0;
    int length;
    int byte_count = 0;

    while (!incomingData.empty() && (incomingData[0] == '\r' || incomingData[0] == '\n')) {
        incomingData.erase(0, 1);
    }

    if (incomingData.empty()) {
        return false;
    }

    if (incomingData[0] == '*') {

        while (byte_count < incomingData.size() && incomingData[byte_count] != '\n') {
            byte_count++;
        }

        if (byte_count == incomingData.size()) {
            return false;
        }

        if (incomingData[byte_count] == '\n') {
            args = std::stoi(incomingData.substr(1, byte_count - 2));
        }
    }

    if (args <= 0) {
        return false;
    }

    byte_count++;

    int remain_args = args;
    int arg_len;

    while (remain_args > 0) {
        if (byte_count < incomingData.size() && incomingData[byte_count] == '$') {

            int line_start = byte_count;

            while (byte_count < incomingData.size() && incomingData[byte_count] != '\n') {
                byte_count++;
            }

            if (byte_count == incomingData.size()) {
                break;
            }

            if (incomingData[byte_count] == '\n') {
                arg_len = std::stoi(incomingData.substr(line_start + 1, byte_count - line_start - 2));
            }

            byte_count++;
                        
            if (byte_count + arg_len + 2 <= incomingData.size()) {
                byte_count += arg_len + 2;
            }

            else {
                break;
            }

            remain_args--;

        }

        else {
            break;
        }
    }

    if (remain_args != 0) {
        return false;
    }

    std::cout << "[FRAME FOUND]\n";
                
    full_input = incomingData.substr(0,byte_count);
    incomingData.erase(0,byte_count);

    return true;
}