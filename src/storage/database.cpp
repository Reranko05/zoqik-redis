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