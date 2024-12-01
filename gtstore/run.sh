#/bin/bash

make clean
make

./bin/start_service --nodes 2 --rep 2 & 
SERVICE_PID=$!
sleep 5
# Launch the client testing app
# Usage: ./test_app <test> <client_id>
# ./bin/test_app  1 &
./bin/test_app single_set_get 1 &
./bin/test_app single_set_get 2 &
./bin/test_app single_set_get 3 &
