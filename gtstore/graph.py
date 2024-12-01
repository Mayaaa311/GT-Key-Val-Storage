import matplotlib.pyplot as plt
import pandas as pd

# Data for load balance chart
data_load_balance = {
    "Node ID": [7, 6, 5, 4, 3, 2, 1],
    "Number of Keys": [14284, 14284, 14284, 14285, 14285, 14285, 14285]
}

# Convert to DataFrame
df_load_balance = pd.DataFrame(data_load_balance)

# Create the load balance bar chart
plt.figure(figsize=(10, 6))
bars = plt.bar(df_load_balance["Node ID"], df_load_balance["Number of Keys"], color='skyblue', edgecolor='black')

# Add data labels
for bar in bars:
    height = bar.get_height()
    plt.text(bar.get_x() + bar.get_width() / 2, height, f'{height}', ha='center', va='bottom', fontsize=10)

# Adding title and labels
plt.title('Number of Keys per Node (Experiment: 7 Nodes, 100,000 PUT Requests)', fontsize=14)
plt.xlabel('Node ID', fontsize=12)
plt.ylabel('Number of Keys', fontsize=12)

# Add gridlines for better readability
plt.grid(axis='y', linestyle='--', alpha=0.7)

# Save and display the graph
plt.xticks(df_load_balance["Node ID"])
plt.tight_layout()
plt.savefig("loadbalance.png")
plt.show()

# Data for throughput test
replicas = [1, 3, 5]  # Number of replicas
throughput = [232.484, 163.639, 123.022]  # Throughput in Ops/s

# Create the throughput bar chart
plt.figure(figsize=(8, 6))
bars = plt.bar(replicas, throughput, color='lightgreen', edgecolor='black')

# Add data labels
for bar in bars:
    height = bar.get_height()
    plt.text(bar.get_x() + bar.get_width() / 2, height, f'{height:.3f}', ha='center', va='bottom', fontsize=10)

# Adding title and labels
plt.title('Throughput vs. Number of Replicas (7 Nodes, 200K Ops)', fontsize=14)
plt.xlabel('Number of Replicas', fontsize=12)
plt.ylabel('Throughput (Ops/s)', fontsize=12)

# Add gridlines for better readability
plt.grid(axis='y', linestyle='--', alpha=0.7)

# Save and display the chart
plt.xticks(replicas)
plt.tight_layout()
plt.savefig("throughput.png")
plt.show()
