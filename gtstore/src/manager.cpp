#include "gtstore.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <deque>
#include <string.h> // For char array operations
#include <arpa/inet.h> // For sockets
#include <unistd.h> // For close()

using namespace std;

// Constructor and initialization method
void GTStoreManager::init(int num_storage, int replica) {
    // Start TCP and UDP sockets for communication
    cout << "Initializing GTStoreManager with " << num_storage 
         << " storage nodes and " << replica << " replicas per key." << endl;

    // Create GTStoreStorage instances and store them in addr_to_st
    // Insert initial data into vacant_storage
    // Start threads for heartbeat, request handling, and acknowledgment
}

// Heartbeat listening loop
void GTStoreManager::listen_heartbeat() {
    cout << "Listening for heartbeats from storage nodes..." << endl;

    // UDP listening loop to monitor heartbeat messages
    // If a node's heartbeat stops, trigger backup and call re_replicate()
}

// Client request listener
void GTStoreManager::listen_request() {
    cout << "Listening for client requests..." << endl;

    // TCP listening loop to handle client requests
    // Dispatch requests to put_request() or get_request() based on message type
}

// Handle put request
void GTStoreManager::put_request(int key, int val) {
    cout << "Handling PUT request for key: " << key << ", value: " << val << endl;

    // Load balancing: Find the most vacant nodes from vacant_storage
    // Update key_node_map with selected storage nodes
    // Send key-value pairs to selected nodes via TCP
    // Respond to the client with the storage node information
}

// Handle get request
void GTStoreManager::get_request(int key) {
    cout << "Handling GET request for key: " << key << endl;

    // Look up key in key_node_map to find associated storage nodes
    // Send storage node information to the client via TCP
}

// Handle node failure and replication
void GTStoreManager::re_replicate() {
    cout << "Handling node failure and re-replication..." << endl;

    // Identify failed node(s) and remove them from key_node_map
    // Create new replicas for lost data
    // Notify the client about updated key-node mappings
}
