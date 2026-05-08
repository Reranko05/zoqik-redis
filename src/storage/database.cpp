/*
    Implements the Database storage engine operations.

    Core responsibilities:
    - cache insertion and updates
    - TTL expiration handling
    - LRU ordering maintenance
    - eviction handling
    - memory ownership cleanup
    - cache state debugging

    Destructor:
    Frees all dynamically allocated nodes,
    including:
    - cache nodes
    - dummy head node
    - dummy tail node

    Traversal cleanup is performed using:
    current -> next node iteration

    to safely avoid dereferencing freed memory.

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

    If cache capacity is exceeded,
    the least recently used node is evicted.

    p_set():
    Inserts or updates persistent cache entries
    without expiration metadata.

    If a previously expiring key becomes persistent,
    the old TTL metadata is removed from expiry storage.

    get():
    Performs:
    - key lookup
    - TTL validation
    - LRU update

    Successful lookups move the accessed node
    to the MRU position.

    Expired keys are removed using lazy expiration:
    - remove node from linked list
    - erase hashmap entry
    - erase expiry metadata
    - free allocated node memory

    before returning "key not found".

    del():
    Removes:
    - linked list node
    - hashmap entry
    - expiry metadata
    - allocated node memory

    to maintain synchronization between
    all internal storage structures.

    insertFront():
    Inserts a node directly after head,
    marking it as Most Recently Used (MRU).

    removeNode():
    Removes a node from the linked list
    in O(1) time by reconnecting neighboring nodes.

    Dummy head and tail nodes are protected
    from accidental removal.

    moveToFront():
    Repositions an existing node
    to the MRU position.

    evictLRU():
    Removes the least recently used node
    when cache capacity is exceeded.

    Eviction removes:
    - linked list node
    - hashmap entry
    - expiry metadata
    - dynamically allocated memory

    printLRU():
    Debug utility used to visualize
    current cache ordering.

    Displays:
    - MRU -> LRU traversal
    - current cache size
    - configured cache capacity

    std::chrono::steady_clock is used because it is
    monotonic and unaffected by system clock changes.
*/
#include "database.h"

#include <chrono>

Database::~Database() {
    Node* current = head;
    while (current != nullptr) {
        Node* nextNode = current->next;
        delete current;
        current = nextNode;
    }
}

void Database::set(const std::string& key, const std::string &value, int ttl) {
    auto it = store.find(key);

    if (it != store.end()) {
        Node* existing = it->second;
        existing->data = value;
        moveToFront(existing);
    }
    else {
        Node* val_node = new Node;
        val_node->data = value;
        val_node->key = key;
        insertFront(val_node);
        store[key] = val_node;
        currentSize++;
    }
    if (capacity < currentSize) {
        evictLRU();
    }
    expiry[key] = Clock::now() + std::chrono::seconds(ttl);
}

void Database::p_set(const std::string& key, const std::string& value) {
    auto it = store.find(key);

    if (it != store.end()) {
        Node* existing = it->second;
        existing->data = value;
        moveToFront(existing);
    }
    else {
        Node* val_node = new Node;
        val_node->data = value;
        val_node->key = key;
        insertFront(val_node);
        store[key] = val_node;
        currentSize++;
    }
    if (capacity < currentSize) {
        evictLRU();
    }
    if (expiry.find(key) != expiry.end()) {
        expiry.erase(key);
    }
}

std::string Database::get(const std::string& key) {
    auto it = store.find(key);
    auto ttl_it = expiry.find(key);

    if (it != store.end() && ttl_it != expiry.end()) {
        if (ttl_it->second >= Clock::now()) {
            moveToFront(it->second);
            return it->second->data;
        }
        else {
            removeNode(it->second);
            delete it->second;
            expiry.erase(ttl_it);
            store.erase(it);
            currentSize--;
            return "[ERROR] key not found";
        }
    } 
    else if (it != store.end() && ttl_it == expiry.end()) {
        moveToFront(it->second);
        return it->second->data;
    }
    else {
        return "[ERROR] key not found";
    }
}   

void Database::del(const std::string& key) {
    auto it = store.find(key);

    if (it != store.end()) {
        removeNode(it->second);
        delete it->second;
        store.erase(it);
        expiry.erase(key);
        currentSize--;
    }

}

void Database::insertFront(Node* node) {
    Node* dummy = head;
    Node* temp = dummy->next;
    
    dummy->next = node;
    node->next = temp;
    node->prev = dummy;
    temp->prev = node;
}

void Database::evictLRU() {
    Node* lru = tail->prev;

    removeNode(lru);
    store.erase(lru->key);
    expiry.erase(lru->key);

    delete lru;
    currentSize--;
}

void Database::removeNode(Node* node) {
    if (node != head && node != tail) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
}

void Database::moveToFront(Node* node) {
    removeNode(node);
    insertFront(node);
}

void Database::printLRU() {
    Node* current = head->next;
    int c = 0;
    std::cout << "MRU -> ";
    while (current != tail) {
        std::cout << "[" << current->key << "] -> ";
        current = current->next;
        c++;
    }
    std::cout << "LRU\n";
    std::cout << "Size: " << c << "/" << capacity << '\n';
}

