// #include "gtstore.hpp"

// void GTStoreStorage::init() {
	
// 	cout << "Inside GTStoreStorage::init()\n";
// }

// int main(int argc, char **argv) {

// 	GTStoreStorage storage;
// 	storage.init();
	
// }


// GTStoreStorage.cpp
#include "gtstore.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <chrono>

#define MANAGER_IP "127.0.0.1"

void GTStoreStorage::init(int port, int id) {
    std::cout << "GTStoreStorage binding to " << port << "\n";
	storage_id = id;
		// Start listening on TCP socket
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0) {
        std::cerr << "Error creating TCP socket" << std::endl;
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    if (setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting socket options" << std::endl;
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in storage_addr;
    memset(&storage_addr, 0, sizeof(storage_addr));
    storage_addr.sin_family = AF_INET;
    storage_addr.sin_addr.s_addr = INADDR_ANY;
    storage_addr.sin_port = htons(port);

    if (::bind(tcp_socket, (struct sockaddr*)&storage_addr, sizeof(storage_addr)) < 0) {
        std::cerr << "Error binding TCP socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (listen(tcp_socket, 5) < 0) {
        std::cerr << "Error listening on TCP socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Send message to manager to inform that storage node is ready
    int manager_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (manager_socket < 0) {
        std::cerr << "Error creating socket to manager" << std::endl;
        return;
    }
    struct sockaddr_in manager_addr;
    memset(&manager_addr, 0, sizeof(manager_addr));
    manager_addr.sin_family = AF_INET;
    manager_addr.sin_port = htons(MANAGER_TCP_PORT);
    if (inet_pton(AF_INET, MANAGER_IP, &manager_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address for manager" << std::endl;
        close(manager_socket);
        return;
    }

    if (connect(manager_socket, (struct sockaddr*)&manager_addr, sizeof(manager_addr)) < 0) {
        std::cerr << "Connection to manager failed" << std::endl;
        close(manager_socket);
        return;
    }

    // Send storage node info to manager
    std::string registration_msg = "STORAGE_NODE_READY " + std::to_string(port);
    send(manager_socket, registration_msg.c_str(), registration_msg.size(), 0);
    // close(manager_socket);

    // Keep manager socket open and start a listener thread
    // std::thread manager_thread(&GTStoreStorage::listen_to_manager, this, manager_socket);
    // manager_thread.detach();

    // Start thread to send heartbeat messages
    std::thread heartbeat_thread(&GTStoreStorage::send_heartbeat, this);
    heartbeat_thread.detach();

    while (true) {
        int client_socket = accept(tcp_socket, NULL, NULL);
        if (client_socket >= 0) {
            // Start a new thread to handle the client request
            std::thread client_thread(&GTStoreStorage::handle_client, this, client_socket);
            client_thread.detach();
        } else {
            std::cerr << "Error accepting client connection: " << strerror(errno) << std::endl;
        }
    }
}

void GTStoreStorage::accept_connections() {
    while (true) {
        int client_socket = accept(tcp_socket, NULL, NULL);
        if (client_socket >= 0) {
            // Start a new thread to handle the client request
            std::thread client_thread(&GTStoreStorage::handle_client, this, client_socket);
            client_thread.detach();
        } else {
            std::cerr << "Error accepting client connection: " << strerror(errno) << std::endl;
        }
    }
}

void GTStoreStorage::handle_client(int client_socket) {
    char buffer[2048];
    ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        std::string request(buffer);
        std::istringstream iss(request);
        std::string command, key;
        iss >> command >> key;
        if (command == "GET") {
            // Handle GET request
            get_request(client_socket, key);
        } 
		else if (command == "PUT") {
			// Handle PUT request
			std::string value;
			val_t vals;
			while(iss >> value){
				vals.push_back(value);
			}
			put_request(key, vals);
			// Optionally send acknowledgment
		} 
        else {
            std::cerr << "Unknown command from client: " << command << std::endl;
        }
    }
    close(client_socket);
}

void GTStoreStorage::put_request(std::string key, val_t value) {
    std::lock_guard<std::mutex> lock(mtx);
    key_val_map[key] = value;
	
    std::cout << "Stored key: " << key << " value: ";
	for(auto i : value){
		cout<<i;
	}
	cout<<endl;
}

void GTStoreStorage::get_request(int client_socket, std::string key) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = key_val_map.find(key);
    if (it != key_val_map.end()) {
        // Send value back to client
        val_t value = it->second;

		std::string msg = std::to_string(storage_id);
		for(auto i : value){
			msg+=" "+i;
		}
        send(client_socket, msg.c_str(), msg.size(), 0);
    } else {
        std::string response = "Error: Key not found";
        send(client_socket, response.c_str(), response.size(), 0);
    }
}

void GTStoreStorage::send_heartbeat() {
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        std::cerr << "Error creating UDP socket for heartbeat" << std::endl;
        return;
    }

    struct sockaddr_in manager_addr;
    memset(&manager_addr, 0, sizeof(manager_addr));
    manager_addr.sin_family = AF_INET;
    manager_addr.sin_port = htons(MANAGER_UDP_PORT);
    if (inet_pton(AF_INET, MANAGER_IP, &manager_addr.sin_addr) <= 0) {
        std::cerr << "Invalid manager IP address for heartbeat" << std::endl;
        close(udp_socket);
        return;
    }

    while (true) {
        std::string heartbeat_msg = "HEARTBEAT";
        sendto(udp_socket, heartbeat_msg.c_str(), heartbeat_msg.size(), 0,
               (struct sockaddr*)&manager_addr, sizeof(manager_addr));
        std::this_thread::sleep_for(std::chrono::seconds(HEARTBEAT_INTERVAL));
    }
    close(udp_socket);
}


void GTStoreStorage::listen_to_manager(int manager_socket) {
    char buffer[2048];
    while (true) {
        ssize_t bytes_read = recv(manager_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string message(buffer);

            // Process manager message
                buffer[bytes_read] = '\0';
                std::string request(buffer);
                std::istringstream iss(request);
                std::string command, key;
                iss >> command >> key;
                if (command == "PUT") {
                    // Handle PUT request
                    std::string value;
					val_t vals;
					while(iss >> value){
						vals.push_back(value);
					}
                    put_request(key, vals);
                    // Optionally send acknowledgment
                } 
                else {
                    std::cerr << "Unknown command from manager:" << command <<"."<< std::endl;
                }
            
            // Add logic to handle specific commands from the manager
        } else if (bytes_read == 0) {
            std::cerr << "Manager disconnected" << std::endl;
            close(manager_socket);
            break;
        } else {
            std::cerr << "Error receiving from manager: " << strerror(errno) << std::endl;
        }
    }
    close(manager_socket);
}
