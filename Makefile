CC = gcc
CFLAGS = -Wall -Wextra -I./include
LDFLAGS = -lm

SRC_DIR = src
BUILD_DIR = build
SUBDIRS = cpu memory disk process config

# Source files
SRCS = $(wildcard $(SRC_DIR)/*/*.c) $(SRC_DIR)/main.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Target executable
TARGET = $(BUILD_DIR)/system_monitor

.PHONY: all clean

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/cpu
	mkdir -p $(BUILD_DIR)/memory
	mkdir -p $(BUILD_DIR)/disk
	mkdir -p $(BUILD_DIR)/process
	mkdir -p $(BUILD_DIR)/config

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
