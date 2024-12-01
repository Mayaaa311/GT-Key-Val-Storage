#!/bin/bash

# Define the list of ports
ports=(8000 8001 8002 8003 8004 8005 8006 8007 8008 8009 8010 8011 8012 8013 8014 8015 8016 8017 8018 8019 8020 8021 8022 8023 8024 8025 8026)

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