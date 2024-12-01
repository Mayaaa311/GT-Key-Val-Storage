#include "gtstore.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include <unordered_map>
void single_set_get(int client_id) {
		cout << "Testing single set-get for GTStore by client " << client_id << ".\n";

		GTStoreClient client;
		client.init();

		string key = to_string(client_id);
		val_t value;
		value.push_back("phone");
		value.push_back("phone_case");

		client.put(key, value);
		client.get(key);

		client.finalize();
}
// Function to perform throughput test
void throughput_test(int num_operations, const string& output_file, int num_replicas) {
    cout << "Starting throughput test: " << num_operations 
         << " operations with " << num_replicas << " replicas.\n";

    GTStoreClient client;
    client.init(); // No need to pass replicas; backend handles it.

    auto start_time = chrono::high_resolution_clock::now();

    for (int i = 0; i < num_operations; i++) {
        string key = "key_" + to_string(i);
        val_t value = { "value_" + to_string(i) };

        if (i % 2 == 0) { // Write operation
            client.put(key, value);
        } else {          // Read operation
            client.get("key_" + to_string(i-1));
        }
		// usleep(1000);
    }

    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end_time - start_time;

    double throughput = num_operations / duration.count(); // Operations per second

    client.finalize();

    // Save results to CSV
    ofstream csv_file(output_file, ios::app);
    if (csv_file.is_open()) {
        csv_file << num_replicas << "," << throughput << "\n";
        csv_file.close();
    } else {
        cerr << "Failed to open file for writing: " << output_file << "\n";
    }

    cout << "Throughput test completed: " << throughput << " Ops/s\n";
}

// Function to perform load balance test
void loadbalance_test(int num_inserts, const string& output_file) {
    cout << "Starting load balance test: " << num_inserts << " inserts.\n";

    GTStoreClient client;
    client.init();

    unordered_map<int, int> node_load; // Track the number of keys per node

    for (int i = 0; i < num_inserts; i++) {
        string key = "key_" + to_string(i);
        val_t value = { "value_" + to_string(i) };

        // Perform put operation and get the list of storage nodes
        vector<int> assigned_nodes = client.put(key, value);

        // Update node load for each assigned storage node
        for (int node_id : assigned_nodes) {
            node_load[node_id]++;
        }
    }

    client.finalize();

    // Save histogram to CSV
    ofstream csv_file(output_file, ios::out);
    if (csv_file.is_open()) {
        csv_file << "Node ID,Number of Keys\n";
        for (const auto& [node_id, count] : node_load) {
            csv_file << node_id << "," << count << "\n";
        }
        csv_file.close();
    } else {
        cerr << "Failed to open file for writing: " << output_file << "\n";
    }

    cout << "Load balance test completed. Histogram saved to " << output_file << "\n";
}

int main(int argc, char **argv) {
		string test = string(argv[1]);
		int client_id = atoi(argv[2]);

		string test1 = "single_set_get";
		string test2 = "throughput";
		string test3 = "lb";
		if (string(argv[1]) ==  test1) {
				single_set_get(client_id);
		}
		else if (string(argv[1]) ==  test2) {
			throughput_test(200000,"throughput.csv",5);
		}
		else{
			loadbalance_test(100000, "loadbalance.csv");
		}

}

