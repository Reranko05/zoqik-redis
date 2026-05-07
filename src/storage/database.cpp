/*
    Implements in-memory key-value storage layer using unordered_map.

    set() method adds or overwrites key-value pairs in storage.

    get() method uses an iterator to lookup the desired key.
    If found, it returns the stored value.
    Otherwise, it returns "Key Not Found".

    get() uses only one hashmap lookup for both validation
    and value retrieval.

    del() method checks whether the key exists in storage.
    If found, it erases the key-value pair from storage.
*/

#include "database.h"

void Database::set(const std::string& key, const std::string& value) {
    store[key] = value;
}

std::string Database::get(const std::string& key) {
    
    auto it = store.find(key);

    if (it != store.end()) {
        return it->second;
    } 
    else {
        return "Key Not Found";
    }
}   

void Database::del(const std::string& key) {

    if (store.find(key) != store.end()) {
        store.erase(key);
    }

}