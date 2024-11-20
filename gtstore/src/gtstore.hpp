#ifndef GTSTORE
#define GTSTORE

#include <string>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <deque>
#define PORT 8080

#define MAX_KEY_BYTE_PER_REQUEST 20
#define MAX_VALUE_BYTE_PER_REQUEST 1000

using namespace std;

typedef vector<string> val_t;
struct addr{

};

class GTStoreClient {
		private:
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
		private:
			int status;
			// key to list of node id
			unordered_map<char[MAX_KEY_BYTE_PER_REQUEST], vector<addr>> key_node_map;

			// easy to get top k storage node
			unordered_map<addr, GTStoreStorage> addr_to_st;
			deque<deque<addr>> vacant_storage;

		public:
				/** 
				 *  1. starting TCP amd UDP socket for manager storage communication
				 *  2. create desginated number of GTStoreStorage, pass tcp and udp PORT to it, insert into storage map
				 *  3. start heartbeat thread to receive heartbeat(UDP), start thread to receive get and put requests(TCP), 
				 *  and start thread for listening for ack (UDP)
				 * */ 
				void init(int num_storage, int replica);
				/**
				 * 1. a loop to keep lisning to heartbeat, once heartbeat stop, call backup() to reduplicate data
				 * 
				 */
				void listen_heartbeat();
				/**
				 * 1. listen to cleint's request
				 * 
				 */
				void listen_request();
				/**
				 * 1. LB: find most vacant k nodes, update node map
				 * 2. send TCP message to every selected node of key and value
				 * 3. send message to client, indicating storage node
				 */
				void put_request(int key, int val);

				/**
				 * 1. Send TCP message of the map to client
				 */
				void get_request(int key);


				/**
				 * 1. delete node 
				 * 2. create new replica
				 * 3. send new map client
				 */
				void re_replicate();

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
