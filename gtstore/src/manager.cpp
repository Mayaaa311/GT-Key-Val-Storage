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

// Custom address structure


// Hash function for NodeAddress
namespace std {
    template <>
    struct hash<NodeAddress> {
        std::size_t operator()(const NodeAddress& k) const {
            return std::hash<std::string>()(k.ip) ^ (std::hash<int>()(k.port) << 1);
        }
    };
}

// Constructor and initialization method
void GTStoreManager::init(int num_storage, int replica) {
    // -----------------Start TCP and UDP sockets for communication----------------------
    cout << "Initializing GTStoreManager with " << num_storage 
        << " storage nodes and " << replica << " replicas per key." << endl;

    this->replica = replica;

    // Initialize TCP socket
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0) {
        cerr << "Error creating TCP socket" << endl;
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        cerr << "Error setting TCP socket options" << endl;
        exit(EXIT_FAILURE);
    }

    // Bind TCP socket
    struct sockaddr_in manager_addr;
    memset(&manager_addr, 0, sizeof(manager_addr));
    manager_addr.sin_family = AF_INET;
    manager_addr.sin_addr.s_addr = INADDR_ANY; // Bind to any interface
    manager_addr.sin_port = htons(MANAGER_TCP_PORT);

    if (bind(tcp_socket, (struct sockaddr*)&manager_addr, sizeof(manager_addr)) < 0) {
        cerr << "Error binding TCP socket" << endl;
        exit(EXIT_FAILURE);
    }

    // Listen on TCP socket
    if (listen(tcp_socket, 5) < 0) {
        cerr << "Error listening on TCP socket" << endl;
        exit(EXIT_FAILURE);
    }

    // Initialize UDP socket
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        cerr << "Error creating UDP socket" << endl;
        exit(EXIT_FAILURE);
    }
    // -----------------Start TCP and UDP sockets for communication----------------------
    cout << "Initializing GTStoreManager with " << num_storage 
        << " storage nodes and " << replica << " replicas per key." << endl;

    this->replica = replica;

    // Initialize TCP socket
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0) {
        cerr << "Error creating TCP socket" << endl;
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        cerr << "Error setting TCP socket options" << endl;
        exit(EXIT_FAILURE);
    }

    // Bind TCP socket
    struct sockaddr_in manager_addr;
    memset(&manager_addr, 0, sizeof(manager_addr));
    manager_addr.sin_family = AF_INET;
    manager_addr.sin_addr.s_addr = INADDR_ANY; // Bind to any interface
    manager_addr.sin_port = htons(MANAGER_TCP_PORT);

    if (bind(tcp_socket, (struct sockaddr*)&manager_addr, sizeof(manager_addr)) < 0) {
        cerr << "Error binding TCP socket" << endl;
        exit(EXIT_FAILURE);
    }

    // Listen on TCP socket
    if (listen(tcp_socket, 5) < 0) {
        cerr << "Error listening on TCP socket" << endl;
        exit(EXIT_FAILURE);
    }
    // -----------------Start TCP and UDP sockets for client communication----------------------

    // Initialize TCP socket
    client_tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_tcp_socket < 0) {
        cerr << "Error creating TCP socket" << endl;
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(client_tcp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        cerr << "Error setting TCP socket options" << endl;
        exit(EXIT_FAILURE);
    }

    // Bind TCP socket
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY; // Bind to any interface
    client_addr.sin_port = htons(CLIENT_TCP_PORT);

    if (bind(client_tcp_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        cerr << "Error binding TCP socket" << endl;
        exit(EXIT_FAILURE);
    }

    // Listen on TCP socket
    if (listen(client_tcp_socket, 5) < 0) {
        cerr << "Error listening on TCP socket" << endl;
        exit(EXIT_FAILURE);
    }

    // Initialize UDP socket------------------------------------------------------------------
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        cerr << "Error creating UDP socket" << endl;
        exit(EXIT_FAILURE);
    }

    // Bind UDP socket
    memset(&manager_addr, 0, sizeof(manager_addr));
    manager_addr.sin_family = AF_INET;
    manager_addr.sin_addr.s_addr = INADDR_ANY;
    manager_addr.sin_port = htons(MANAGER_UDP_PORT);

    if (bind(udp_socket, (struct sockaddr*)&manager_addr, sizeof(manager_addr)) < 0) {
        cerr << "Error binding UDP socket" << endl;
        exit(EXIT_FAILURE);
    }

    // Create storage nodes
    int base_port = STORAGE_NODE_BASE_PORT;
    vacant_storage.push_back({});
    for (int i = 0; i < num_storage; ++i) {
        GTStoreStorage storage_node;
        int storage_port = base_port + i;
        storage_node.init(storage_port);

        NodeAddress node_addr = {"127.0.0.1", storage_port};
        // addr_to_st[node_addr] = storage_node;
        // Start a thread to monitor this node's heartbeat
        thread heartbeat_thread(&GTStoreManager::monitor_node_heartbeat, this, node_addr);
        heartbeat_thread.detach();

        // Add to vacant storage
        vacant_storage.back().push_back(node_addr);
    }

    // Create GTStoreStorage instances and store them in addr_to_st
    // Insert initial data into vacant_storage
    // Start threads for heartbeat, request handling, and acknowledgment


    // Startcthreads
    thread request_thread(&GTStoreManager::listen_request, this);

    // Detach threads
    heartbeat_thread.detach();
    request_thread.detach();
}

// Heartbeat listening loop
void GTStoreManager::listen_heartbeat(NodeAddress node_addr) {
    // cout << "Listening for heartbeats from storage nodes..." << endl;

    // UDP listening loop to monitor heartbeat messages
    // If a node's heartbeat stops, trigger backup and call re_replicate()
    cout << "Listening for heartbeat from node " << node_addr.ip << ":" << node_addr.port << endl;

    char buffer[2048];
    struct sockaddr_in storage_addr;
    socklen_t addr_len = sizeof(storage_addr);

    // Set timeout for recvfrom
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (flag == 1) {
        int bytes_received = recvfrom(udp_socket, buffer, sizeof(buffer), 0,
                                      (struct sockaddr*)&storage_addr, &addr_len);

        if (bytes_received > 0) {
            // Successfully received heartbeat
            cout << "Heartbeat received from " << node_addr.ip << ":" << node_addr.port << endl;
        } else {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // Timeout occurred, assume node failure
                cerr << "Node failed: " << node_addr.ip << ":" << node_addr.port << endl;

                // loop through vacant storage and delete the dead node
                for(auto i = vacant_storage.begin(); i!= vacant_storage.end();i++
                ){
                    auto it = find(i->begin(), i->end(), node_addr);
                    if(it != i->end()){
                        i->erase(it);


                        if(i->empty()){
                            vacant_storage.earse(i);
                        }
                        break;
                    }
                }
                re_replicate(node_addr);
                return; // Exit the thread for this node
            } else {
                cerr << "Error receiving heartbeat from " << node_addr.ip << ":" << node_addr.port
                     << ": " << strerror(errno) << endl;
                return; // Exit the thread for this node
            }
        }
    }
    
}

// Client request listener
void GTStoreManager::listen_request() {
    // TCP listening loop to handle client requests
    // Dispatch requests to put_request() or get_request() based on message type
    cout << "Listening for client requests..." << endl;

    while (flag == 1) {
        int client_tcp_socket = accept(tcp_socket, NULL, NULL);
        if (client_tcp_socket >= 0) {
            // Start a new thread to handle the client request
            thread client_thread([this, client_tcp_socket]() {
                char request_buffer[2048];
                ssize_t bytes_read = recv(client_tcp_socket, request_buffer, sizeof(request_buffer) - 1,0);
                if (bytes_read > 0) {
                    // Null-terminate the buffer
                    request_buffer[bytes_read] = '\0';
                    // Parse the request
                    string request(request_buffer);
                    istringstream iss(request);
                    string request_type;
                    iss >> request_type;
                    if (request_type == "PUT") {
                        // Extract key and value
                        int key, val;
                        iss >> key >> val;
                        this->put_request(key, val);
                    } else if (request_type == "GET") {
                        int key;
                        iss >> key;
                        this->get_request(key);
                    } 
                    else {
                       
                        string response = "Error: Unknown request type";
                        cerr<<response<<endl;
                    }
                }
                close(client_tcp_socket);
            });
            client_thread.detach();
        } else {
            cerr << "Error accepting client connection: " << strerror(errno) << endl;
        }
    }
}

// Handle put request
void GTStoreManager::put_request(int key, int val) {
    cout << "Handling PUT request for key: " << key << ", value: " << val << endl;

    // Load balancing: Find the most vacant nodes from vacant_storage
    // Update key_node_map with selected storage nodes
    // Send key-value pairs to selected nodes via TCP
    // Respond to the client with the storage node information
    
    // Select storage nodes for replication


    //TODO: Add case for changing value
    vector<NodeAddress> selected_nodes;
    lock_guard<mutex> lock(mtx);
    if(key_node_map.find(key) != key_node_map.end()){
        selected_nodes = key_node_map[key];
    }
    else{
        for (int i = 0; i < replica; ++i) {
            if (!vacant_storage.empty()) {
                NodeAddress node_addr = vacant_storage.front().front();
                //check if there is only one node
                if(vacant_storage.size() > 1 ||  vacant_storage.front.size() >1){
                    vacant_storage.front().pop_front();
                    //check if this node is the only node in previous queue
                    if (vacant_storage.front().empty()) {
                        vacant_storage.pop_front();
                    }
                    //check if all the node are currently in one queue
                    else if(vacant_storage.front() == vacant_stroage.back()){
                        vacant_storage.push_back({});
                    }
                    vacant_storage.back().push_back(node_addr);
                }
            } 
            else {
                string response = "Error: No available storage nodes";
                cerr<<response<<endl;
                send(client_tcp_socket, response.c_str(), response.size(), 0);
                return;
            }
        }
    }

    // Update key-node mapping
    key_node_map[key] = selected_nodes;

    // Send key-value pair to selected storage nodes
    for (const auto& node_addr : selected_nodes) {
        // Establish TCP connection to storage node
        int storage_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (storage_socket < 0) {
            cerr << "Error creating storage socket" << endl;
            continue;
        }
        struct sockaddr_in storage_addr;
        storage_addr.sin_family = AF_INET;
        storage_addr.sin_port = htons(node_addr.port);
        inet_pton(AF_INET, node_addr.ip.c_str(), &storage_addr.sin_addr);
        if (connect(storage_socket, (struct sockaddr*)&storage_addr, sizeof(storage_addr)) < 0) {
            cerr << "Error connecting to storage node " << node_addr.ip << ":" << node_addr.port << endl;
            close(storage_socket);
            continue;
        }
        // Send PUT request
        string storage_request = "PUT " + to_string(key) + " " + to_string(val);
        send(storage_socket, storage_request.c_str(), storage_request.size(), 0);
        close(storage_socket);
    }

    // Respond to client
    string response = "PUT_Success "+selected_nodes[0].ip.c_str()+" "+selected_nodes[0].port.c_str();
    cout << response<<endl; 
    send(client_tcp_socket, response.c_str(), response.size(), 0);
}

// Handle get request
void GTStoreManager::get_request(int key) {
    cout << "Handling GET request for key: " << key << endl;

    // Look up key in key_node_map to find associated storage nodes
    // Send storage node information to the client via TCP
    lock_guard<mutex> lock(mtx);
    if (key_node_map.find(key) != key_node_map.end()) {
        // Fetch the list of storage nodes for the key
        vector<NodeAddress> storage_nodes = key_node_map[key];

        // Serialize the vector of NodeAddress into a string
        string response;
        for (const auto& node : storage_nodes) {
            response += node.ip + ":" + to_string(node.port) + "\n"; // Format: IP:PORT
        }

        // Send the serialized vector to the client
        send(client_tcp_socket, response.c_str(), response.size(), 0);
    } 
    else {
        // Key not found
        string response = "Error: Key not found";
        cerr<response<<endl;
        send(client_tcp_socket, response.c_str(), response.size(), 0);
    }
}

// Handle node failure and replication
void GTStoreManager::re_replicate(NodeAddress failed_node) {
    // Identify failed node(s) and remove them from key_node_map
    // Create new replicas for lost data
    // Notify the client about updated key-node mappings
    cout << "Handling node failure and re-replication for node " << failed_node.ip << ":" << failed_node.port << endl;

    lock_guard<mutex> lock(mtx);

    // Remove node from key_node_map
    for (auto& [key, nodes] : key_node_map) {
        auto it = find(nodes.begin(), nodes.end(), failed_node);
        if (it != nodes.end()) {
            nodes.erase(it);
            // Need to replicate this key to a new node
            if (!vacant_storage.empty()) {

                //--------finding the new rplica node to replace the failed node-----------------
                NodeAddress new_node_addr = NULL;
                // loop thorugh vacant storage
                for(auto i = vacant_storage.begin(); i!= vacant_storage.end();i++
                ){
                    // loop thorugh each deque in vacant storage in order
                    for(auto j = i->begin();j!= i->end();j++){
                        // find if current storage is already in the key node map
                        auto it = find(nodes.begin(), nodes.end(), *j);

                        // if it is not in the map, make it the new_node_addr and break
                        if(it == nodes.end()){
                            new_node_addr = *j;
                            break;
                        }
                    }
                    if(new_node_addr != NULL)
                        break;
                }

                nodes.push_back(new_node_addr);

                // Send key-value data to new node
                if nodes.size() == 1{
                    cerr<<"THERE's No replica node to reference! Data Lost!";
                    return;
                }
                
                NodeAddress existing_node_addr = nodes.front();

                // Retrieve value from an existing node
                int existing_node_socket = socket(AF_INET, SOCK_STREAM, 0);
                struct sockaddr_in existing_storage_addr;
                existing_storage_addr.sin_family = AF_INET;
                existing_storage_addr.sin_port = htons(existing_node_addr.port);
                inet_pton(AF_INET, existing_node_addr.ip.c_str(), &existing_storage_addr.sin_addr);
                if (connect(existing_node_socket, (struct sockaddr*)&existing_storage_addr, sizeof(existing_storage_addr)) < 0) {
                    cerr << "Error connecting to existing storage node" << endl;
                    close(existing_node_socket);
                    continue;
                }

                // Send GET request
                string storage_request = "GET " + to_string(key);
                send(existing_node_socket, storage_request.c_str(), storage_request.size(), 0);
                // Receive the value
                char value_buffer[2048];
                ssize_t bytes_read = recv(existing_node_socket, value_buffer, sizeof(value_buffer) - 1, 0);
                close(existing_node_socket);

                if (bytes_read <= 0) {
                    cerr << "Error retrieving value from existing storage node" << endl;
                    continue;
                }
                value_buffer[bytes_read] = '\0';

                // Send PUT request to new node
                int new_node_socket = socket(AF_INET, SOCK_STREAM, 0);
                struct sockaddr_in new_storage_addr;
                new_storage_addr.sin_family = AF_INET;
                new_storage_addr.sin_port = htons(new_node_addr.port);
                inet_pton(AF_INET, new_node_addr.ip.c_str(), &new_storage_addr.sin_addr);
                if (connect(new_node_socket, (struct sockaddr*)&new_storage_addr, sizeof(new_storage_addr)) < 0) {
                    cerr << "Error connecting to new storage node" << endl;
                    close(new_node_socket);
                    continue;
                }

                string put_request = "PUT " + to_string(key) + " " + string(value_buffer);
                send(new_node_socket, put_request.c_str(), put_request.size(), 0);
                close(new_node_socket);
            } else {
                cerr << "No available storage nodes to replicate key " << key << endl;
            }
        }
    }
}


