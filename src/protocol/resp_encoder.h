#pragma once

#include <string>

std::string encodeSimpleString(const std::string& input);

std::string encodeBulkString(const std::string& input);

std::string encodeError(const std::string& input);

std::string encodeNil();
