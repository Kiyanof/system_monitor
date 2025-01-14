CC = gcc
CFLAGS = -Wall -Wextra -I./include
LDFLAGS = -lrt -pthread

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Source files
COMMON_SRCS = $(wildcard $(SRC_DIR)/cpu/*.c) \
              $(wildcard $(SRC_DIR)/memory/*.c) \
              $(wildcard $(SRC_DIR)/disk/*.c) \
              $(wildcard $(SRC_DIR)/process/*.c) \
              $(wildcard $(SRC_DIR)/config/*.c) \
              $(wildcard $(SRC_DIR)/ipc/*.c)

COLLECTOR_SRCS = $(SRC_DIR)/collector.c
DISPLAY_SRCS = $(SRC_DIR)/display.c

# Object files
COMMON_OBJS = $(COMMON_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
COLLECTOR_OBJS = $(COLLECTOR_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DISPLAY_OBJS = $(DISPLAY_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Executables
COLLECTOR = $(BIN_DIR)/collector
DISPLAY = $(BIN_DIR)/display

# Targets
.PHONY: all clean directories

all: directories $(COLLECTOR) $(DISPLAY)

directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR) \
		$(OBJ_DIR)/cpu $(OBJ_DIR)/memory $(OBJ_DIR)/disk \
		$(OBJ_DIR)/process $(OBJ_DIR)/config $(OBJ_DIR)/ipc

$(COLLECTOR): $(COMMON_OBJS) $(COLLECTOR_OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(DISPLAY): $(COMMON_OBJS) $(DISPLAY_OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
