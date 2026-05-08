#pragma once

#include <string>
#include <unordered_map>
#include <chrono>

using Clock = std::chrono::steady_clock;

class Database {
public:
    void set(const std::string& key, const std::string& value, int ttl);

    void p_set(const std::string& key, const std::string& value);

    std::string get(const std::string& key);

    void del(const std::string& key);

private:
    std::unordered_map<std::string, std::string> store;
    std::unordered_map<std::string, Clock::time_point> expiry;
};