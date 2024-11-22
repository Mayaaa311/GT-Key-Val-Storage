#include "gtstore.hpp"
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
#define STORAGE_NODE_BASE_PORT 5000
#define HEARTBEAT_INTERVAL 5       // seconds
#define HEARTBEAT_TIMEOUT 10       // seconds

using namespace std;

typedef vector<string> val_t;
struct NodeAddress {
    std::string ip;
    int port;

    bool operator==(const NodeAddress& other) const {
        return ip == other.ip && port == other.port;
    }
};

class GTStoreClient {
		private:
				int tcp_socket; 
				int tcp_socket; 
				int client_id;
				val_t value;
				vector<char[MAX_KEY_BYTE_PER_REQUEST], addr> nodemap;
		public:
				void init(int id);
				void finalize();
				/**
				 * @brief 
				 * connect to manager node, get map
				 * retrieve from node
				 */
				val_t get(string key);
				/**
				 * @brief 
				 * 
				 * @param key 
				 * @param value 
				 * connect to manager
				 */
				bool put(string key, val_t value);
};

class GTStoreManager {
	public:	
		void init(int num_storage, int replica);
	private:
		void listen_heartbeat(NodeAddress node_addr);
		void listen_request();
		void put_request(int key, int val);
		void get_request(int key);
		void re_replicate(NodeAddress failed_node);
		int flag;
		int tcp_socket;
		int client_tcp_socket;
		int udp_socket;
		int replica;
		// unordered_map<NodeAddress, GTStoreStorage> addr_to_st;
		deque<deque<NodeAddress>> vacant_storage;
		unordered_map<int, vector<NodeAddress>> key_node_map;

		mutex mtx;  // Mutex for thread safety
};

class GTStoreStorage {
		private:
			unordered_map<char[MAX_KEY_BYTE_PER_REQUEST], char[MAX_VALUE_BYTE_PER_REQUEST]> key_val_map;

		public:
				/**
				 * 1. fork a process to start storage
				 * 2. connect to manager, send ack for start success message
				 */
				void init();
				void put_request(int key, int val);
				void get_request(int key, int val);
				void send_heartbeat();

};

#endif
