
// GTStoreClient.cpp
#include "gtstore.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define MANAGER_IP "127.0.0.1"

void GTStoreClient::init() {
}

string GTStoreClient::get(const std::string& key) {
	string value;
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
    manager_addr.sin_port = htons(CLIENT_TCP_PORT);
    if (inet_pton(AF_INET, MANAGER_IP, &manager_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address for manager" << std::endl;
        close(manager_socket);
        return value;
    }
    // Connect to manager
    if (connect(manager_socket, (struct sockaddr*)&manager_addr, sizeof(manager_addr)) < 0) {
        std::cerr << "Connection to manager failed, try to get: " <<key<< std::endl;
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
        std::cerr << "Failed to receive response from manager, try to get: "<<key << std::endl;
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
		int storage_id;
        if (line_stream >> storage_ip >> storage_port >> storage_id) {
            storage_nodes.push_back({storage_ip, storage_port, storage_id});
        }
    }

    if (storage_nodes.empty()) {
        std::cerr << "No storage nodes found for key" << std::endl;
        return value;
    }
	else{
		// cout<<"querying storage nodes:";
		// for(auto i: storage_nodes){
		// 	cout<<i.id<<",";
		// }
		// cout<<endl;
	}
    while (!storage_nodes.empty()) {
        NodeAddress storage_node = storage_nodes.front();
        storage_nodes.erase(storage_nodes.begin());

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
        std::string id;
        value_stream >> id;
		std::getline(value_stream >> std::ws, value); 
		cout<<key<<", "<<value<<", server"<<storage_node.id<<endl;

        return value; // Successfully fetched value
    }
    std::cerr << "Failed to retrieve value from all storage nodes" << std::endl;
    return value;
}

vector<int> GTStoreClient::put(const std::string& key, const val_t& value){
    vector<int> result;
    // std::cout << "Inside GTStoreClient::put() for client: " << " key: " << key << " first value: " << value[0] << "\n";

    // Create TCP socket to manager
    int manager_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (manager_socket < 0) {
        std::cerr << "Error creating socket to manager" << std::endl;
        return {};
    }

    // Set up manager address
    struct sockaddr_in manager_addr;
    memset(&manager_addr, 0, sizeof(manager_addr));
    manager_addr.sin_family = AF_INET;
    manager_addr.sin_port = htons(CLIENT_TCP_PORT);



    if (inet_pton(AF_INET, MANAGER_IP, &manager_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address for manager" << std::endl;
        close(manager_socket);
        return {};
    }

    // Connect to manager
    if (connect(manager_socket, (struct sockaddr*)&manager_addr, sizeof(manager_addr)) < 0) {
        std::cerr << "Connection to manager failed" << std::endl;
        close(manager_socket);
        return {};
    }


	// Construct the request string
	std::string request = "PUT " + key;
	for (auto i : value){
		request+=" " + i;
	}
    if (send(manager_socket, request.c_str(), request.size(), 0) < 0) {
        std::cerr << "Failed to send PUT request to manager" << std::endl;
        close(manager_socket);
        return {};
    }

    // Receive response from manager
    char buffer[2048];
	bool success = false;
    while (true) {
        ssize_t bytes_received = recv(manager_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received < 0) {
            std::cerr << "Error receiving response from manager" << std::endl;
            break;
        }

        if (bytes_received == 0) {
            // No more data, the connection is closed by the server
            std::cerr << "Connection closed by manager "<<strerror(errno) << std::endl;
            break;
        }

        buffer[bytes_received] = '\0';
        std::string response(buffer);

        // Parse storage node info from response
        std::istringstream iss(response);
        std::string status, storage_ip;
        string storage_ids;
        
        iss >> status;

        if (status == "PUT_Success") {
            std::cout << "OK, server: ";
            success = true;

            // getline(iss >> std::ws, storage_ids); 
            while(iss >> storage_ids){
                cout<<storage_ids<<" , ";
                result.push_back(stoi(storage_ids));
            }
            cout<<endl;
            break;
        } else {
            std::cerr << "Manager responded with error: " << response << std::endl;
            break;
        }
    }

    close(manager_socket);
    return result;
}

void GTStoreClient::finalize() {
    // std::cout << "Inside GTStoreClient::finalize() for client " << "\n";
}


