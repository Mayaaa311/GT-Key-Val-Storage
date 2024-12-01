#include "gtstore.hpp"
#include <string.h> // For char array operations
#include <arpa/inet.h> // For sockets
#include <unistd.h> // For close()
#include <iostream>
#include <csignal> // For signal handling
GTStoreManager manager; 

void signal_handler(int signum) {
    std::cout << "\nSignal " << signum << " received. Shutting down GTStoreManager..." << std::endl;
    manager.shutdown_manager(); // Call the cleanup function
    exit(EXIT_SUCCESS); // Exit the program gracefully
}

int main(int argc, char* argv[]) {
    // Default values for nodes and replicas
    int num_nodes = 1;
    int num_replicas = 1;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--nodes") == 0 && i + 1 < argc) {
            num_nodes = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--rep") == 0 && i + 1 < argc) {
            num_replicas = atoi(argv[++i]);
        } else {
            cerr << "Usage: " << argv[0] << " --nodes <number_of_nodes> --rep <replication_factor>" << endl;
            return EXIT_FAILURE;
        }
    }

    // Validate inputs
    if (num_nodes <= 0 || num_replicas <= 0) {
        cerr << "Error: Number of nodes and replication factor must be greater than 0." << endl;
        return EXIT_FAILURE;
    }


    // Register the signal handler
    // std::signal(SIGINT, signal_handler); // Handle Ctrl+C
    // std::signal(SIGTERM, signal_handler); // Handle termination signal

    // Initialize GTStoreManager
    cout << "Starting GTStoreManager with " << num_nodes 
         << " storage node(s) and " << num_replicas << " replica(s) per key." << endl;

    manager.init(num_nodes, num_replicas);

    // Keep the main thread alive to manage the service
    cout << "GTStoreManager service is now running..." << endl;
    while (true) {
        this_thread::sleep_for(chrono::seconds(1));
    }

    return EXIT_SUCCESS;
}

