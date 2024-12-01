#include "gtstore.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <deque>
#include <string.h> // For char array operations
#include <arpa/inet.h> // For sockets
#include <unistd.h> // For close()
#include <ctime>
#include <signal.h> // For kill()
#include <fcntl.h>    // Required for fcntl and FD_CLOEXEC
using namespace std;

// Custom address structure

#define MAX_RETRIES 5
#define RETRY_DELAY_MS 100 // 100 milliseconds


// Constructor and initialization method
void GTStoreManager::init(int num_storage, int replica) {
    // -----------------Start TCP and UDP sockets for communication----------------------
    cout << "Initializing GTStoreManager with " << num_storage 
        << " storage nodes and " << replica << " replicas per key!!!!" << endl;

    this->replica = replica;
	flag = 1;
	int opt = 1;
    // -----------------Start TCP and UDP sockets for client communication----------------------

    // Initialize TCP socket
    client_tcp_socket = socket(AF_INET, SOCK_STREAM , 0);
    if (client_tcp_socket < 0) {
        cerr << "Error creating TCP socket" << endl;
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(client_tcp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        cerr << "Error setting TCP socket options" << endl;
        exit(EXIT_FAILURE);
    }

    int flags = fcntl(client_tcp_socket, F_GETFD);
    if (flags == -1) {
        std::cerr << "Error getting socket flags" << std::endl;
        return ;
    }

    flags |= FD_CLOEXEC;
    if (fcntl(client_tcp_socket, F_SETFD, flags) == -1) {
        std::cerr << "Error setting FD_CLOEXEC" << std::endl;
        return ;
    }

    // if (setsockopt(client_tcp_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    //     std::cerr << "Error setting socket timeout" << std::endl;
    // }
    // Bind TCP socket
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY; // Bind to any interface
    client_addr.sin_port = htons(CLIENT_TCP_PORT);

    if (::bind(client_tcp_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        cerr << "Error binding TCP socket for client tcp" << endl;
        exit(EXIT_FAILURE);
    }

    // Listen on TCP socket
    if (listen(client_tcp_socket, 5) < 0) {
        cerr << "Error listening on TCP socket" << endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "GTStoreManager TCP for client binding to " << CLIENT_TCP_PORT << " for socket "<<client_tcp_socket<<"\n";
    
    // Initialize UDP socket------------------------------------------------------------------
	struct sockaddr_in manager_addr;
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
	std::cout << "GTStoreManager UDP for heartbeat binding to " << MANAGER_UDP_PORT << "\n";
	opt = 1;
	if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		cerr << "Error setting UDP socket options" << endl;
		exit(EXIT_FAILURE);
	}

    if (::bind(udp_socket, (struct sockaddr*)&manager_addr, sizeof(manager_addr)) < 0) {
        cerr << "Error binding UDP socket: " << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }
    // Create storage nodes--------------------------------------------------------------------

    thread listen_heartbeat_thread(&GTStoreManager::listen_heartbeat, this);
    listen_heartbeat_thread.detach();



	int base_port = STORAGE_NODE_BASE_PORT;
	vacant_storage.push_back({});
    vector<int> storage_sockets;
    cout<<"--------------------------------------------------------"<<endl;
    cout<<"--------------------------------------------------------"<<endl;
    int strgid = 1;
	for (int i = 0; i < num_storage*2; i+=2) {
		pid_t pid = fork();
	
		if (pid < 0) {
			std::cerr << "Error: Failed to fork process for storage node " << strgid << std::endl;
			exit(EXIT_FAILURE);
		} 
        else if (pid == 0) { 
			// Child process: Initialize and run the storage node
    		std::cout << "Child Process (PID: " << getpid() << ") started for storage node with ID " << strgid << std::endl;
			int storage_port = base_port + i;
			GTStoreStorage storage_node;
			storage_node.init(storage_port, strgid);

			exit(0); // Exit the child process after storage node execution
		} 
        else {

            sleep(1);
			// Parent process: Track the storage node
            int storage_TCP_port = base_port + i;
            
            NodeAddress node_addr = {"127.0.0.1", storage_TCP_port, strgid};

            forked_processes.push_back(pid);
			// Start a thread to monitor this node's heartbeat
			time_t last_beat = time(nullptr);
            heartbeat_map[node_addr.id] = last_beat;
            vacant_storage.back().push_back(node_addr);
            thread monitor_heartbeat_thread(&GTStoreManager::monitor_heartbeat, this, node_addr);
            monitor_heartbeat_thread.detach();	

		}
        strgid++;
	}

    // Create GTStoreStorage instances and store them in addr_to_st
    // Insert initial data into vacant_storage
    // Start threads for heartbeat, request handling, and acknowledgment


    // Startcthreads
    thread request_thread(&GTStoreManager::listen_request, this);

    // Detach threads
    request_thread.detach();



    while(flag == 1){
        sleep(5);
    }

    for(auto socket: storage_sockets){

        close(socket);
    }

}
void GTStoreManager::listen_heartbeat(){
    char buffer[1024];
    struct sockaddr_in storage_addr;
    socklen_t addr_len = sizeof(storage_addr);

    time_t cur_beat;
    while (flag == 1) {
        int bytes_received = recvfrom(udp_socket, buffer, sizeof(buffer), 0,
                                      (struct sockaddr*)&storage_addr, &addr_len);
            cur_beat = time(nullptr); 
        if (bytes_received > 0) {
            // Successfully received heartbeat
            int cur_id = std::stoi(buffer);
            heartbeat_map[cur_id] = cur_beat;

            // cout << "Heartbeat received from " << cur_id << endl;


        } 
    }
}
void GTStoreManager::monitor_heartbeat(NodeAddress node_addr){
    time_t cur_time= time(nullptr);
    while(flag == 1){
        cur_time= time(nullptr);
        if(cur_time-heartbeat_map[node_addr.id] > HEARTBEAT_INTERVAL*2){
            // Timeout occurred, assume node failure
            cerr << "Node failed: " << node_addr.id << endl;
            re_replicate(node_addr);
            return; // Exit the thread for this node
        } 
        // cout<<"heartbeat interval; "<<heartbeat_map[node_addr.id]- cur_time<<endl;
        this_thread::sleep_for(chrono::seconds(1));
    }
}


// Client request listener
void GTStoreManager::listen_request() {
    // TCP listening loop to handle client requests
    // Dispatch requests to put_request() or get_request() based on message type
    cout << "Listening for client requests..." << endl;

    while (flag == 1) {
        int client_fd = accept(client_tcp_socket, NULL, NULL);
        if (client_fd >= 0) {
            // Start a new thread to handle the client request
            thread client_thread([this, client_fd]() {
                char request_buffer[2048];
                ssize_t bytes_read = recv(client_fd, request_buffer, sizeof(request_buffer) - 1,0);
                if (bytes_read > 0) {
                    cout<<"RECEIVED msg in manager from client: "<<request_buffer<<" in socket: "<<client_fd<<endl;
                    // Null-terminate the buffer
                    request_buffer[bytes_read] = '\0';
                    // Parse the request
                    string request(request_buffer);
                    istringstream iss(request);
                    string request_type;
                    iss >> request_type;
                    if (request_type == "PUT") {
                        // Extract key and value
                        string key;
						string val;
						val_t vals;
                        iss >> key;
						while(iss >> val){
							vals.push_back(val);
						}
                        this->put_request(key, vals, client_fd);
                    } else if (request_type == "GET") {
                        string key;
                        iss >> key;
                        this->get_request(key,client_fd);
                    } 
                    else {
                       
                        string response = "Error: Unknown request type";
                        cerr<<response<<endl;
                    }
                    cout<<"--------------------------------------------------------"<<endl;
                }
                
            });
            client_thread.detach();
        } else {
            // cerr << "Error accepting client connection: " << strerror(errno) << endl;
        }
        
    }
}
NodeAddress GTStoreManager::select_node(){
    if (!vacant_storage.empty()) {
        NodeAddress node_addr = vacant_storage.front().front();
        //check if there is only one node
        if(vacant_storage.size() > 1 ||  vacant_storage.front().size() >1){ //---------------------------------------
            vacant_storage.front().pop_front();
            //check if this node is the only node in previous queue
            if (vacant_storage.front().empty()) {
                vacant_storage.pop_front();
            }
            //check if all the node are currently in one queue
            else if(vacant_storage.front() == vacant_storage.back()){
                vacant_storage.push_back({});
            }
            vacant_storage.back().push_back(node_addr);
            return node_addr;
        }
        else {
            return node_addr;
        }
    } 

    return {"-1",-1,-1};

}

void GTStoreManager::put_request(std::string key, val_t val,int client_fd) {
    cout << "PUT: Handling PUT request for key: " << key << ", value starting with : " << val[0] << endl;

    // Load balancing: Find the most vacant nodes from vacant_storage
    // Update key_node_map with selected storage nodes
    // Send key-value pairs to selected nodes via TCP
    // Respond to the client with the storage node information
    
    // Select storage nodes for replication


    //TODO: Add case for changing value
    vector<NodeAddress> selected_nodes;
    // lock_guard<mutex> lock(mtx);
    if(key_node_map.find(key) != key_node_map.end()){
        selected_nodes = key_node_map[key];
    }
    else{
        for (int i = 0; i < replica; ++i) {
            NodeAddress selection = select_node();
            if(selection.id != -1)
                selected_nodes.push_back(selection);
            else{
                string response = "Error: Not enough storage nodes";
                cerr<<response<<endl;
                break;
            }
        }
    }

    // Update key-node mapping
    key_node_map[key] = selected_nodes;
	NodeAddress selected_node;
	string response = "PUT_Success ";
    // Send key-value pair to selected storage nodes
    for (const auto& node_addr : selected_nodes) {
		// cout<<"Puting node to storage ndoe: "<<node_addr.id<<endl;
        // Establish TCP connection to storage node

        // Send PUT request
        int storage_socket = socket(AF_INET, SOCK_STREAM, 0);
        int retries = 0;

        // Retry loop for creating the socket
        while (storage_socket < 0 && retries < MAX_RETRIES) {
            storage_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (storage_socket < 0) {
                std::cerr << "Error creating storage socket (attempt "
                        << retries + 1 << "): " << strerror(errno) << std::endl;
                retries++;
                if (retries < MAX_RETRIES) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
                }
            }
        }

        // storage_socket = tcp_socket;

        struct sockaddr_in storage_addr;
        storage_addr.sin_family = AF_INET;
        storage_addr.sin_port = htons(node_addr.port);
        inet_pton(AF_INET, node_addr.ip.c_str(), &storage_addr.sin_addr);
        while (connect(storage_socket, (struct sockaddr*)&storage_addr, sizeof(storage_addr)) < 0) {
            cerr << "Error connecting to storage node :" << node_addr.id <<"  - "<<strerror(errno)<< endl;
            re_replicate(node_addr);
            close(storage_socket);
            continue;
        }

        string storage_request = "PUT " + key;
		for(auto i : val){
			storage_request+=" "+i;
		}
        send(storage_socket, storage_request.c_str(), storage_request.size(), 0);
        close(storage_socket);

		response+=to_string(node_addr.id)+" ";
    }
    cout<<"PUT: Sent response to client send: "<<response<<endl;
    send(client_fd, response.c_str(), response.size(), 0);
    close(client_fd);
	
}

// Handle get request
void GTStoreManager::get_request(std::string key,int client_fd) {
    cout << "MANAGER: Handling GET request for key: " << key << endl;

    // Look up key in key_node_map to find associated storage nodes
    // Send storage node information to the client via TCP
    // lock_guard<mutex> lock(mtx);
    if (key_node_map.find(key) != key_node_map.end()) {
        // Fetch the list of storage nodes for the key
        vector<NodeAddress> storage_nodes = key_node_map[key];

        // Serialize the vector of NodeAddress into a string
        string response;
        for (const auto& node : storage_nodes) {
            response += node.ip + " " + to_string(node.port) + " " + to_string(node.id) +"\n"; // Format: IP:PORT
        }
        // Send the serialized vector to the client
        send(client_fd, response.c_str(), response.size(), 0);
         cout<<"GET: Sent node list to client: "<<response<<endl;
    } 
    else {
        // Key not found
        string response = "Error: Key not found";
        cerr<<response<<endl;
        send(client_fd, response.c_str(), response.size(), 0);
    }
    close(client_fd);
}

// Handle node failure and replication
void GTStoreManager::re_replicate(NodeAddress failed_node) {
    // Identify failed node(s) and remove them from key_node_map
    // Create new replicas for lost data
    // Notify the client about updated key-node mappings
    cout << "NODE_FAILURE: Handling node failure and re-replication for node " << failed_node.ip << ":" << failed_node.port << endl;

    lock_guard<mutex> lock(mtx);
    // Handle node failure and remove it from vacant storage
    for (auto i = vacant_storage.begin(); i != vacant_storage.end(); ++i) {
        auto it = find(i->begin(), i->end(), failed_node);
        if (it != i->end()) {
            i->erase(it);

            if (i->empty()) {
                vacant_storage.erase(i);
            }
            break;
        }
    }
    // Remove node from key_node_map
    for (auto& [key, nodes] : key_node_map) {
        auto it = find(nodes.begin(), nodes.end(), failed_node);
        if (it != nodes.end()) {
            nodes.erase(it);
            // Need to replicate this key to a new node
            if (!vacant_storage.empty()) {

                //--------finding the new rplica node to replace the failed node-----------------
                NodeAddress new_node_addr;  
				bool found = false;         // To track if a new node is found

				// Loop through vacant storage
				for (auto i = vacant_storage.begin(); i != vacant_storage.end(); i++) {
					for (auto j = i->begin(); j != i->end(); j++) {
						// Check if the storage is already in the key node map
						auto it = find(nodes.begin(), nodes.end(), *j);

						// If it is not in the map, assign it and break
						if (it == nodes.end()) {
							new_node_addr = *j;  // Assign found node
							found = true;        // Mark as found
							break;
						}
					}
					if (found) break;  // Exit outer loop if a node is found
				}

				// After the loop, check if `new_node_addr` was assigned
				if (!found) {
					std::cerr << "No suitable node found!" << std::endl;
					// Handle the error (e.g., return or throw an exception)
				}

                nodes.push_back(new_node_addr);

                // Send key-value data to new node
                if (nodes.size() == 1){
                    cerr<<"NODE_FAILURE: THERE's No replica node to reference! Data Lost!";
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
                string storage_request = "GET " + key;
                send(existing_node_socket, storage_request.c_str(), storage_request.size(), 0);
                // Receive the value
                char value_buffer[2048];
                ssize_t bytes_read = recv(existing_node_socket, value_buffer, sizeof(value_buffer) - 1, 0);
                close(existing_node_socket);

                if (bytes_read <= 0) {
                    cerr << "Error retrieving value from existing storage node" << endl;
                    continue;
                }


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

                string request(value_buffer);
                istringstream iss(request);
                string id;
                iss >> id;
                std::string value;
                std::getline(iss >> std::ws, value); 

                string put_request = "PUT " + key + " " + value;
                send(new_node_socket, put_request.c_str(), put_request.size(), 0);
                close(new_node_socket);
            } else {
                cerr << "No available storage nodes to replicate key " << key << endl;
            }
        }
    }
}


void GTStoreManager::shutdown_manager() {
    flag = 0; // Set the flag to stop loops
    if (client_tcp_socket >= 0) {
        close(client_tcp_socket);
        cout << "Closed client TCP socket." << endl;
    }
    if (udp_socket >= 0) {
        close(udp_socket);
        cout << "Closed UDP socket." << endl;
    }
    // Terminate all forked processes
    for (pid_t pid : forked_processes) {
        if (pid > 0) { // Check for a valid PID
            cout << "Terminating child process with PID: " << pid << endl;
            if (kill(pid, SIGTERM) == 0) {
                cout << "Successfully terminated process " << pid << endl;
            } else {
                cerr << "Failed to terminate process " << pid << ": " << strerror(errno) << endl;
            }
        }
    }
}