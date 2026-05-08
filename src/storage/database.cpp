/*
    Implements in-memory key-value storage layer using unordered_map.

    Storage is divided into two hashmaps:

    1. store
       Maps:
       key -> value

       Stores the actual key-value data.

    2. expiry
       Maps:
       key -> expiration timestamp

       Stores TTL metadata for expiring keys
       using std::chrono::steady_clock::time_point.

    p_set() method adds or overwrites persistent
    key-value pairs in storage.

    If the key previously had expiration metadata,
    it removes the old expiry entry to maintain
    storage consistency.

    set() method adds or overwrites expiring
    key-value pairs in storage.

    It stores expiration timestamps as:
    current_time + ttl_seconds

    using std::chrono::steady_clock.

    get() method uses iterators to lookup:
    - key existence
    - expiration metadata

    If:
    - key exists
    - key has expiration metadata

    then current time is compared against
    the stored expiration timestamp.

    If the key is expired:
    - key-value pair is removed from storage
    - expiration metadata is removed

    before returning "key not found".

    This is called lazy expiration.

    If:
    - key exists
    - no expiration metadata exists

    then key is treated as persistent.

    get() uses only one hashmap lookup per structure
    for validation and retrieval.

    del() method checks whether the key exists in storage.

    If found, it erases:
    - key-value pair from storage
    - expiration metadata from expiry hashmap

    to maintain synchronization between both structures.

    std::chrono::steady_clock is used because it is
    monotonic and unaffected by system clock changes.
*/

#include "database.h"

#include <chrono>

void Database::set(const std::string& key, const std::string& value, int ttl) {
    store[key] = value;
    expiry[key] = Clock::now() + std::chrono::seconds(ttl);
}

void Database::p_set(const std::string& key, const std::string& value) {
    if (expiry.find(key) != expiry.end()) {
        expiry.erase(key);
    }
    store[key] = value;
}

std::string Database::get(const std::string& key) {
    
    auto it = store.find(key);
    auto ttl_it = expiry.find(key);

    if (it != store.end() && ttl_it != expiry.end()) {
        if (ttl_it->second >= Clock::now()) {
            return it->second;
        }
        else {
            expiry.erase(key);
            store.erase(key);
            return "[ERROR] key not found";
        }
    } 
    else if (it != store.end() && ttl_it == expiry.end()) {
        return it->second;
    }
    else {
        return "[ERROR] key not found";
    }
}   

void Database::del(const std::string& key) {

    if (store.find(key) != store.end()) {
        store.erase(key);
        expiry.erase(key);
    }

}