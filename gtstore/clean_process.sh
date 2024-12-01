#!/bin/bash

# Define the list of ports
ports=(8000 8001 8002 8003 8004 8005 8006 8007 8008 8009 8010)

# Loop through each port
for port in "${ports[@]}"; do
  echo "Checking port: $port"
  
  # Find the process ID (PID) using lsof
  pid=$(lsof -t -i :$port)
  
  # If a PID is found, kill the process
  if [ -n "$pid" ]; then
    echo "Port $port is in use by PID $pid. Killing process..."
    kill -9 $pid
    echo "Process $pid killed."
  else
    echo "Port $port is not in use."
  fi
done
