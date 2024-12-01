#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <deque>
#include <string>
#include <sstream>
#include <cstring>       // For char array operations
#include <arpa/inet.h>   // For sockets
#include <unistd.h>      // For close()
#include <chrono>
#include <mutex>
#include <netinet/in.h>
#include <errno.h>

#define MANAGER_UDP_PORT 8001
#define CLIENT_TCP_PORT 8002
#define STORAGE_NODE_BASE_PORT 8005
#define HEARTBEAT_INTERVAL 2      // seconds

using namespace std;

typedef vector<string> val_t;
// Hash function for NodeAddress
// namespace std {
//     template <>
//     struct hash<NodeAddress> {
//         std::size_t operator()(const NodeAddress& k) const {
//             return std::hash<std::string>()(k.ip) ^ (std::hash<int>()(k.port) << 1);
//         }
//     };
// }
struct NodeAddress {
    std::string ip;
    int storage_manager_port;
	int storage_client_port;
	int id;
	int socket;

    bool operator==(const NodeAddress& other) const {
        return id == other.id;
    }
};

class GTStoreClient {
		private:
				// int tcp_socket; 
				unordered_map<string, NodeAddress> nodemap;
		public:
			string get(const std::string& key); 
			vector<int> put(const std::string& key, const val_t& value); 

			void init();
			void finalize();
};

class GTStoreManager {
	public:	
		void init(int num_storage, int replica);
		void shutdown_manager();

	private:
		void listen_heartbeat();
		void monitor_heartbeat(NodeAddress node_addr);
		void listen_request();
		void put_request(std::string key, val_t val,int c);
		void get_request(std::string key,int c);
		void re_replicate(NodeAddress failed_node);
		vector<pid_t> forked_processes;
		int flag;
		int tcp_socket;
		int client_tcp_socket;
		int udp_socket;
		int replica;
		unordered_map<int,time_t> heartbeat_map;
		deque<deque<NodeAddress>> vacant_storage;
		unordered_map<std::string, vector<NodeAddress>> key_node_map;

		mutex mtx;  // Mutex for thread safety
};

class GTStoreStorage {
		private:
			unordered_map<string, val_t> key_val_map;
			int storage_id;
			int tcp_socket;
			int tcp_socket_client;
			mutex mtx;  // Mutex for thread safety
			int running;
		public:
				/**
				 * 1. fork a process to start storage
				 * 2. connect to manager, send ack for start success message
				 */
				void init(int port, int id);
				void put_request(string key, val_t val);
				void get_request(int client_socket, std::string key);
				void listen_to_manager(int manager_socket);
				void send_heartbeat();
				void accept_connections(int socket);
				void handle_client(int client_socket, bool close_conn);

};

