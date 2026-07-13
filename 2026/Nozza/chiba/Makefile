# Compiler and flags 
CC 	    := gcc
CFLAGS  := -Wall -Wextra -O2 -g -Iinclude
TARGET  := bin/chiba
# Link external libraries
LDFLAGS := -lcurl -lpthread 

# Directories
SRC_DIR := src
OBJ_DIR := build
BIN_DIR := bin

# Find source files in src/ and object files in build/
SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

.PHONY: clean

all: $(TARGET)

# Create directories before compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -rf  $(OBJ_DIR) $(BIN_DIR)