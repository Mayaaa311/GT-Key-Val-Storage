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

#define MANAGER_TCP_PORT 4000
#define MANAGER_UDP_PORT 4001
#define CLIENT_TCP_PORT 4002
#define STORAGE_NODE_BASE_PORT 4005
#define HEARTBEAT_INTERVAL 5       // seconds
#define HEARTBEAT_TIMEOUT 10       // seconds

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
    int port;
	int id;

    bool operator==(const NodeAddress& other) const {
        return ip == other.ip && port == other.port;
    }
};

class GTStoreClient {
		private:
				// int tcp_socket; 
				unordered_map<string, NodeAddress> nodemap;
		public:
			string get(const std::string& key); 
			bool put(const std::string& key, const val_t& value); 

			void init();
			void finalize();
};

class GTStoreManager {
	public:	
		void init(int num_storage, int replica);
		
	private:
		void listen_heartbeat(NodeAddress node_addr);
		void listen_request();
		void put_request(std::string key, val_t val,int c);
		void get_request(std::string key,int c);
		void re_replicate(NodeAddress failed_node);
		int flag;
		int tcp_socket;
		int client_tcp_socket;
		int udp_socket;
		int replica;
		deque<deque<NodeAddress>> vacant_storage;
		unordered_map<std::string, vector<NodeAddress>> key_node_map;

		mutex mtx;  // Mutex for thread safety
};

class GTStoreStorage {
		private:
			unordered_map<string, val_t> key_val_map;
			int storage_id;
			int tcp_socket;
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
				void accept_connections();
				void handle_client(int client_socket);

};

