#/bin/bash

make clean
make

./start_service --nodes 7 --rep 1 & 
SERVICE_PID=$!
sleep 10
# Launch the client testing app
# Usage: ./test_app <test> <client_id>
./bin/test_app lb 1 &
# ./bin/test_app single_set_get 2 &
# ./bin/test_app single_set_get 3 
