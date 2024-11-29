
// GTStoreClient.cpp
#include "gtstore.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage:" << std::endl;
        std::cerr << "./client --put <key> --val <value>" << std::endl;
        std::cerr << "./client --get <key>" << std::endl;
        return 1;
    }

    GTStoreClient client;
    client.init(); // Initialize the client with ID 1 (or any ID you want)

    std::string command = argv[1];

    if (command == "--put") {
        if (argc != 5 || std::string(argv[3]) != "--val") {
            std::cerr << "Invalid usage for PUT command. Correct usage:" << std::endl;
            std::cerr << "./client --put <key> --val <value>" << std::endl;
            return 1;
        }

        char key[20];
        char value[1024];
        std::strncpy(key, argv[2], sizeof(key) - 1);
        key[sizeof(key) - 1] = '\0'; // Ensure null-terminated
        std::strncpy(value, argv[4], sizeof(value) - 1);
        value[sizeof(value) - 1] = '\0'; // Ensure null-terminated
		std::istringstream iss(value);
		val_t vals;
		string val;
		while(iss>>val){
			vals.push_back(val);
		}
		
        if (client.put(key, vals)) {
        } else {
            std::cerr << "PUT request failed." << std::endl;
        }

    } else if (command == "--get") {
        if (argc != 3) {
            std::cerr << "Invalid usage for GET command. Correct usage:" << std::endl;
            std::cerr << "./client --get <key>" << std::endl;
            return 1;
        }

        std::string key = argv[2];
        char value[1024];

        std::memset(value, 0, sizeof(value)); // Initialize value with zeros

        std::string result = client.get(key);


    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        std::cerr << "Usage:" << std::endl;
        std::cerr << "./client --put <key> --val <value>" << std::endl;
        std::cerr << "./client --get <key>" << std::endl;
        return 1;
    }

    client.finalize(); // Finalize the client
    return 0;
}