/*
    Implements storage operations for the Database engine.

    Core responsibilities:
    - key insertion and updates
    - TTL expiration handling
    - LRU ordering maintenance
    - eviction handling
    - memory cleanup

    set():
    Inserts or updates expiring cache entries.

    Existing keys:
    - update stored value
    - move node to MRU position

    New keys:
    - allocate new node
    - insert into linked list
    - index in hashmap

    Expiration timestamps are stored as:
    current_time + ttl_seconds

    using std::chrono::steady_clock.

    p_set():
    Inserts or updates persistent cache entries
    without expiration metadata.

    If an existing key previously contained TTL metadata,
    the expiry entry is removed to convert the key
    into a persistent entry.

    get():
    Performs:
    - key lookup
    - TTL validation
    - LRU update

    Expired keys are removed using lazy expiration:
    - remove node from linked list
    - erase hashmap entry
    - erase expiry metadata
    - free allocated memory

    Successful lookups move the accessed node
    to the MRU position.

    del():
    Removes:
    - linked list node
    - hashmap entry
    - expiry metadata
    - allocated node memory

    insertFront():
    Inserts a node directly after head.

    removeNode():
    Removes a node from the linked list
    in O(1) time.

    moveToFront():
    Repositions an existing node
    to the MRU position.

    evictLRU():
    Removes the least recently used node
    when cache capacity is exceeded.
*/

#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <iostream>

using Clock = std::chrono::steady_clock;

/*
    data stores the cached value associated
    with a key inside the LRU node.
*/

struct Node{
    std::string key;
    std::string data;
    Node* prev;
    Node* next;
};

class Database {
public:

    Database() {
        head->next = tail;
        tail->prev = head;
    }

    ~Database();

    void set(const std::string& key, const std::string& value, int ttl);

    void p_set(const std::string& key, const std::string& value);

    std::string get(const std::string& key);

    void del(const std::string& key);

    void insertFront(Node* node);

    void removeNode(Node* node);

    void moveToFront(Node* node);

    void evictLRU();

    void printLRU();

private:
    std::unordered_map<std::string, Node*> store;
    std::unordered_map<std::string, Clock::time_point> expiry;
    int capacity = 3;
    int currentSize = 0;
    Node* head = new Node;
    Node* tail = new Node;
};