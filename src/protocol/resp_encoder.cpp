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