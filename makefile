# Compiler settings
CC       = gcc
CFLAGS   = -Wall -Wextra -g -Iinclude $(shell pkg-config --cflags libprotobuf-c)
LDFLAGS  = $(shell pkg-config --libs libprotobuf-c) -lpthread

# Directory layout
SRC_DIR     = source
TEST_DIR    = tests
INCLUDE_DIR = include
OBJ_DIR     = object
BIN_DIR     = binary
LIB_DIR     = lib
PROTO_DIR   = .

# Protocol Buffers
PROTOC_C = protoc-c
PROTO_FILE = $(PROTO_DIR)/sdmessage.proto
PROTO_C = $(SRC_DIR)/sdmessage.pb-c.c
PROTO_H = $(INCLUDE_DIR)/sdmessage.pb-c.h
PROTO_OBJ = $(OBJ_DIR)/sdmessage.pb-c.o

# Main programs (add or remove as needed)
CLIENT_MAIN = list_client
SERVER_MAIN = list_server

# Discover which mains actually exist
CLIENT_SRC := $(wildcard $(SRC_DIR)/$(CLIENT_MAIN).c)
SERVER_SRC := $(wildcard $(SRC_DIR)/$(SERVER_MAIN).c)

# Library sources (only data.c and list.c as per assignment)
LIB_SRCS := $(SRC_DIR)/data.c $(SRC_DIR)/list.c
LIB_OBJS := $(OBJ_DIR)/data.o $(OBJ_DIR)/list.o

# Discover source files (excluding the main files and library files)
ALL_SRCS := $(wildcard $(SRC_DIR)/*.c)
ALL_SRCS := $(filter-out $(PROTO_C),$(ALL_SRCS))
MAIN_SRCS := $(CLIENT_SRC) $(SERVER_SRC)
MODULE_SRCS := $(filter-out $(MAIN_SRCS) $(LIB_SRCS),$(ALL_SRCS))

# Discover test files
TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)

# Generate object file paths
MODULE_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(MODULE_SRCS))
CLIENT_OBJ := $(OBJ_DIR)/$(CLIENT_MAIN).o
SERVER_OBJ := $(OBJ_DIR)/$(SERVER_MAIN).o
TEST_OBJS := $(patsubst $(TEST_DIR)/%.c,$(OBJ_DIR)/%.o,$(TEST_SRCS))

# Binary paths
CLIENT_BIN := $(BIN_DIR)/$(CLIENT_MAIN)
SERVER_BIN := $(BIN_DIR)/$(SERVER_MAIN)
TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(BIN_DIR)/%,$(TEST_SRCS))

# Determine which targets to build based on which sources exist
TARGETS :=
ifneq ($(CLIENT_SRC),)
TARGETS += list_client
endif
ifneq ($(SERVER_SRC),)
TARGETS += list_server
endif

# Library
LIBLIST = $(LIB_DIR)/liblist.a

# Default target - builds whatever mains exist
all: $(TARGETS)

# Generate Protocol Buffers files
proto: $(PROTO_C) $(PROTO_H)

$(PROTO_C) $(PROTO_H): $(PROTO_FILE)
	@echo "Generating Protocol Buffers files..."
	@mkdir -p $(SRC_DIR) $(INCLUDE_DIR)
	$(PROTOC_C) --c_out=$(SRC_DIR) --proto_path=$(PROTO_DIR) $(PROTO_FILE)
	@mv $(SRC_DIR)/sdmessage.pb-c.h $(INCLUDE_DIR)/ 2>/dev/null || true
	@echo "Protocol Buffers generated"

$(PROTO_OBJ): $(PROTO_C) $(PROTO_H)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $(PROTO_C) -o $@

# Create output directories if they don't exist
dir_check:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)

# Build the library (only data.o and list.o as per assignment)
liblist: dir_check proto $(LIBLIST)

$(LIBLIST): $(LIB_OBJS)
	@echo "Creating library $@..."
	ar -rcs $@ $(LIB_OBJS)

# Build client (only if source exists)
ifneq ($(CLIENT_SRC),)
list_client: dir_check proto $(LIBLIST) $(CLIENT_BIN)

$(CLIENT_BIN): $(CLIENT_OBJ) $(MODULE_OBJS) $(PROTO_OBJ) $(LIBLIST)
	@echo "Building client..."
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJ) $(MODULE_OBJS) $(PROTO_OBJ) -L$(LIB_DIR) -llist $(LDFLAGS)
else
list_client:
	@echo "Error: $(SRC_DIR)/$(CLIENT_MAIN).c not found"
	@exit 1
endif

# Build server (only if source exists)
ifneq ($(SERVER_SRC),)
list_server: dir_check proto $(LIBLIST) $(SERVER_BIN)

$(SERVER_BIN): $(SERVER_OBJ) $(MODULE_OBJS) $(PROTO_OBJ) $(LIBLIST)
	@echo "Building server..."
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJ) $(MODULE_OBJS) $(PROTO_OBJ) -L$(LIB_DIR) -llist $(LDFLAGS)
else
list_server:
	@echo "Error: $(SRC_DIR)/$(SERVER_MAIN).c not found"
	@exit 1
endif

# Build tests
tests: dir_check proto $(LIBLIST) $(TEST_BINS)

# Build each test program
$(BIN_DIR)/%: $(OBJ_DIR)/%.o $(MODULE_OBJS) $(PROTO_OBJ) $(LIBLIST)
	@echo "Building test $@..."
	$(CC) $(CFLAGS) -o $@ $< $(MODULE_OBJS) $(PROTO_OBJ) -L$(LIB_DIR) -llist $(LDFLAGS)

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(PROTO_H)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile test files to object files
$(OBJ_DIR)/%.o: $(TEST_DIR)/%.c $(PROTO_H)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Run all tests
test: tests
	@for test in $(TEST_BINS); do \
		echo "Running test: $${test}"; \
		./$$test; \
		echo; \
	done

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)

# Clean protocol buffers
clean-proto:
	rm -f $(PROTO_C) $(PROTO_H) $(PROTO_OBJ)

.PHONY: all proto dir_check liblist list_client list_server tests test clean clean-proto
