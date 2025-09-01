CC := gcc
AR := ar
CFLAGS := -Wall -Wextra -O2 -fPIC -Iinclude -pthread -Iuidq/server

SRC_DIR := uidq/server
CORE_DIR := $(SRC_DIR)/core
MASTER_DIR := $(SRC_DIR)/master
PROCESS_DIR := $(SRC_DIR)/process
UVENT_DIR := $(SRC_DIR)/uvent
UDOP_DIR := $(SRC_DIR)/udop
INCLUDE_DST := include/uidq

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
LIB_DIR := $(BUILD_DIR)/lib
BIN_DIR := $(BUILD_DIR)/bin

TARGET_LIB := $(LIB_DIR)/libuidq.a
TARGET_MASTER := $(BIN_DIR)/uidq
TARGET_PROCESS := $(BIN_DIR)/process_worker

COMMON_SRCS := $(wildcard $(CORE_DIR)/*.c) $(wildcard $(UVENT_DIR)/*.c) $(wildcard $(UDOP_DIR)/*.c)
MASTER_SRCS := $(wildcard $(MASTER_DIR)/*.c)
PROCESS_SRCS := $(wildcard $(PROCESS_DIR)/*.c)

COMMON_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(COMMON_SRCS))
MASTER_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(MASTER_SRCS))
PROCESS_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(PROCESS_SRCS))

.PHONY: all clean headers

all: headers $(TARGET_LIB) $(TARGET_MASTER) $(TARGET_PROCESS)

headers:
	@mkdir -p $(INCLUDE_DST)
	@rsync -a --include '*/' --include '*.h' --exclude '*' $(SRC_DIR)/ $(INCLUDE_DST)/

$(TARGET_LIB): $(COMMON_OBJS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_MASTER): $(MASTER_OBJS) $(TARGET_LIB)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -L$(LIB_DIR) -luidq -o $@

$(TARGET_PROCESS): $(PROCESS_OBJS) $(COMMON_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(INCLUDE_DST)/*.h
