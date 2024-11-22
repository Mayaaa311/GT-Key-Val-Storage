// #include "gtstore.hpp"

// void GTStoreClient::init(int id) {

// 		cout << "Inside GTStoreClient::init() for client " << id << "\n";
// 		client_id = id;
// }

// val_t GTStoreClient::get(string key) {

// 		cout << "Inside GTStoreClient::get() for client: " << client_id << " key: " << key << "\n";
// 		val_t value;
// 		// Get the value!
// 		return value;
// }

// bool GTStoreClient::put(string key, val_t value) {

// 		string print_value = "";
// 		for (uint i = 0; i < value.size(); i++) {
// 				print_value += value[i] + " ";
// 		}
// 		cout << "Inside GTStoreClient::put() for client: " << client_id << " key: " << key << " value: " << print_value << "\n";
// 		// Put the value!
// 		return true;
// }

// void GTStoreClient::finalize() {

// 		cout << "Inside GTStoreClient::finalize() for client " << client_id << "\n";
// }



// GTStoreClient.cpp
#include "gtstore.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define MANAGER_IP "127.0.0.1"

void GTStoreClient::init(int id) {
    std::cout << "Inside GTStoreClient::init() for client " << id<<"\n";
    client_id = id;
}

val_t GTStoreClient::get(std::string key) {
    std::cout << "Inside GTStoreClient::get() for client: " << client_id << " key: " << key << "\n";
    val_t value;

    // Create TCP socket to manager
    int manager_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (manager_socket < 0) {
        std::cerr << "Error creating socket to manager" << std::endl;
        return value;
    }

    // Set up manager address
    struct sockaddr_in manager_addr;
    memset(&manager_addr, 0, sizeof(manager_addr));
    manager_addr.sin_family = AF_INET;
    manager_addr.sin_port = htons(MANAGER_TCP_PORT);
    if (inet_pton(AF_INET, MANAGER_IP, &manager_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address for manager" << std::endl;
        close(manager_socket);
        return value;
    }

    // Connect to manager
    if (connect(manager_socket, (struct sockaddr*)&manager_addr, sizeof(manager_addr)) < 0) {
        std::cerr << "Connection to manager failed" << std::endl;
        close(manager_socket);
        return value;
    }

    // Send GET request to manager
    std::string request = "GET " + key;
    if (send(manager_socket, request.c_str(), request.size(), 0) < 0) {
        std::cerr << "Failed to send GET request to manager" << std::endl;
        close(manager_socket);
        return value;
    }

    // Receive response from manager
    char buffer[2048];
    ssize_t bytes_received = recv(manager_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        std::cerr << "Failed to receive response from manager" << std::endl;
        close(manager_socket);
        return value;
    }
    buffer[bytes_received] = '\0';
    std::string response(buffer);
    close(manager_socket);

    // Parse storage node info from response
    std::vector<NodeAddress> storage_nodes;
    std::istringstream iss(response);
    std::string line;
    while (std::getline(iss, line)) {
        std::istringstream line_stream(line);
        std::string storage_ip;
        int storage_port;
        if (line_stream >> storage_ip >> storage_port) {
            storage_nodes.push_back({storage_ip, storage_port});
        }
    }

    if (storage_nodes.empty()) {
        std::cerr << "No storage nodes found for key" << std::endl;
        return value;
    }
    while (!nodes_to_try.empty()) {
        NodeAddress storage_node = nodes_to_try.front();
        nodes_to_try.pop();

        // Connect to storage node
        int storage_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (storage_socket < 0) {
            std::cerr << "Error creating socket for storage node: " << storage_node.ip << ":" << storage_node.port << std::endl;
            continue; // Try the next node
        }

        struct sockaddr_in storage_addr;
        memset(&storage_addr, 0, sizeof(storage_addr));
        storage_addr.sin_family = AF_INET;
        storage_addr.sin_port = htons(storage_node.port);
        if (inet_pton(AF_INET, storage_node.ip.c_str(), &storage_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address for storage node: " << storage_node.ip << ":" << storage_node.port << std::endl;
            close(storage_socket);
            continue; // Try the next node
        }

        if (connect(storage_socket, (struct sockaddr*)&storage_addr, sizeof(storage_addr)) < 0) {
            std::cerr << "Connection to storage node failed: " << storage_node.ip << ":" << storage_node.port << std::endl;
            close(storage_socket);
            continue; // Try the next node
        }

        // Send GET request to storage node
        request = "GET " + key;
        if (send(storage_socket, request.c_str(), request.size(), 0) < 0) {
            std::cerr << "Failed to send GET request to storage node: " << storage_node.ip << ":" << storage_node.port << std::endl;
            close(storage_socket);
            continue; // Try the next node
        }

        // Receive value from storage node
        bytes_received = recv(storage_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            std::cerr << "Failed to receive value from storage node: " << storage_node.ip << ":" << storage_node.port << std::endl;
            close(storage_socket);
            continue; // Try the next node
        }
        buffer[bytes_received] = '\0';
        std::string value_str(buffer);
        close(storage_socket);

        // Parse value string into val_t
        std::istringstream value_stream(value_str);
        std::string val_part;
        while (value_stream >> val_part) {
            value.push_back(val_part);
        }

        return value; // Successfully fetched value
    }
    std::cerr << "Failed to retrieve value from all storage nodes" << std::endl;
    return value;
}

bool GTStoreClient::put(std::string key, val_t value) {
    std::string print_value;
    for (uint i = 0; i < value.size(); i++) {
        print_value += value[i] + " ";
    }
    std::cout << "Inside GTStoreClient::put() for client: " << client_id << " key: " << key << " value: " << print_value << "\n";

    // Create TCP socket to manager
    int manager_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (manager_socket < 0) {
        std::cerr << "Error creating socket to manager" << std::endl;
        return false;
    }

    // Set up manager address
    struct sockaddr_in manager_addr;
    memset(&manager_addr, 0, sizeof(manager_addr));
    manager_addr.sin_family = AF_INET;
    manager_addr.sin_port = htons(MANAGER_TCP_PORT);
    if (inet_pton(AF_INET, MANAGER_IP, &manager_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address for manager" << std::endl;
        close(manager_socket);
        return false;
    }

    // Connect to manager
    if (connect(manager_socket, (struct sockaddr*)&manager_addr, sizeof(manager_addr)) < 0) {
        std::cerr << "Connection to manager failed" << std::endl;
        close(manager_socket);
        return false;
    }

    // Send PUT request to manager
    std::string value_str;
    for (const auto& val_part : value) {
        value_str += val_part + " ";
    }
    std::string request = "PUT " + key + " " + value_str;

    if (send(manager_socket, request.c_str(), request.size(), 0) < 0) {
        std::cerr << "Failed to send PUT request to manager" << std::endl;
        close(manager_socket);
        return false;
    }

    // Receive response from manager
    char buffer[2048];
    ssize_t bytes_received = recv(manager_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        std::cerr << "Failed to receive response from manager" << std::endl;
        close(manager_socket);
        return false;
    }
    buffer[bytes_received] = '\0';
    std::string response(buffer);
    close(manager_socket);

    // Parse storage node info from response
    std::istringstream iss(response);
    std::string status, storage_ip;
    int storage_port;
    iss >> status >> storage_ip >> storage_port;

    if (status != "PUT_Success") {
        std::cerr << "Manager responded with error: " << response << std::endl;
        return false;
    }
    return true;
}

void GTStoreClient::finalize() {
    std::cout << "Inside GTStoreClient::finalize() for client " << client_id << "\n";
}
