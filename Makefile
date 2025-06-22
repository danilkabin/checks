CC := gcc
CFLAGS := -Wall -Wextra -O2 -pthread -lurcu -Iproto/include -Iproto/utils/include -Iproto/https -Iinclude

SRC_DIRS := proto proto/utils/src src
BUILD_DIR := build
TARGET := userspace

SRCS := $(shell find $(SRC_DIRS) -name '*.c')

OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
