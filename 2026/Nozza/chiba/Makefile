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

# ClickHouse HTTP interface
CH_URL  := http://localhost:8123
SQL_DIR := sql

.PHONY: clean db-init db-drop

all: $(TARGET)

db-init:
	@for f in $(SQL_DIR)/*.sql; do \
		echo "[db-init] Applying $$f..."; \
		curl -s --fail --data-binary @$$f $(CH_URL) || exit 1; \
	done

db-drop:
	@echo "[db-drop] WARNING: this will destroy all data in the chiba database."
	@read -p "Type 'yes' to confirm: " confirm && [ "$$confirm" = "yes" ] || exit 1
	@curl -s --fail --data-binary "DROP DATABASE IF EXISTS chiba;" $(CH_URL)

# Create directories before compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -rf  $(OBJ_DIR) $(BIN_DIR)