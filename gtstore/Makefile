CFLAGS  = -std=c++17 -Wall -arch arm64
CC      = g++
RM      = /bin/rm -rf

# Define output directories
BIN_DIR = bin
SRC_DIR = src
OBJ_DIR = obj

# Define targets
SERVER = $(BIN_DIR)/start_service
CLIENT = $(BIN_DIR)/client
TEST_APP = $(BIN_DIR)/test_app

# Source files for each target
SERVER_SRC = $(SRC_DIR)/start_manager_main.cpp $(SRC_DIR)/manager.cpp $(SRC_DIR)/storage.cpp
CLIENT_SRC = $(SRC_DIR)/start_client_main.cpp $(SRC_DIR)/client.cpp
TEST_APP_SRC = $(SRC_DIR)/test_app.cpp $(SRC_DIR)/client.cpp $(SRC_DIR)/manager.cpp $(SRC_DIR)/storage.cpp

# Object files
SERVER_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SERVER_SRC))
CLIENT_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(CLIENT_SRC))
TEST_APP_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(TEST_APP_SRC))

# Default target
all: $(SERVER) $(CLIENT) $(TEST_APP)

# Rule for building the server
$(SERVER): $(SERVER_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Rule for building the client
$(CLIENT): $(CLIENT_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Rule for building the test application
$(TEST_APP): $(TEST_APP_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Rule for compiling source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target to remove binaries and object files
clean:
	$(RM) $(BIN_DIR)/* $(OBJ_DIR)/*

# Phony targets
.PHONY: all clean
