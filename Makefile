CC := ccache gcc
AR := ar
CFLAGS := -Wall -Wextra -O2 -fPIC -Iinclude -Wno-unused-parameter -Wno-unused-variable -Isrc

BUILD_DIR := build
PROCESS_DIR := src/process
MASTER_DIR := src/master
INCLUDE_DST := include/uidq
STATIC_CONFIG := config/config.conf

COMMON_SRC_DIRS := src/core src/config src/uvent
VERSION := $(shell grep '#define UIDQ_VERSION' src/core/uidq_core.h | awk '{print $$3}' | tr -d '"')
VERSION_DIR := $(BUILD_DIR)/$(VERSION)
LIB_DIR := $(VERSION_DIR)/lib
OBJ_DIR := $(VERSION_DIR)/obj
PROCESS_OBJ_DIR := $(VERSION_DIR)/process/obj
BIN_DIR := $(VERSION_DIR)/bin

TARGET_LIB := $(LIB_DIR)/libuidq.a
TARGET_MASTER := $(BIN_DIR)/uidq
TARGET_PROCESS := $(BIN_DIR)/process_worker

COMMON_SRCS := $(shell find $(COMMON_SRC_DIRS) -name '*.c')
COMMON_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(COMMON_SRCS))

MASTER_SRCS := $(shell find $(MASTER_DIR) -name '*.c')
MASTER_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(MASTER_SRCS))

PROCESS_SRCS := $(shell find $(PROCESS_DIR) -name '*.c')
PROCESS_OBJS := $(patsubst %.c,$(PROCESS_OBJ_DIR)/%.o,$(PROCESS_SRCS))

.PHONY: all clean headers

all: $(VERSION_DIR) headers $(TARGET_LIB) $(TARGET_MASTER) $(TARGET_PROCESS)

$(VERSION_DIR):
	@mkdir -p $(VERSION_DIR)
	@cp $(STATIC_CONFIG) $(VERSION_DIR)

headers:
	@mkdir -p $(INCLUDE_DST)
	@rsync -a --include '*/' --include '*.h' --exclude '*' src $(INCLUDE_DST)/

$(TARGET_LIB): $(COMMON_OBJS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(PROCESS_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_MASTER): $(MASTER_OBJS) $(TARGET_LIB)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -L$(LIB_DIR) -luidq -pthread -o $@

$(TARGET_PROCESS): $(PROCESS_OBJS) $(COMMON_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(VERSION_DIR)
	rm -rf $(INCLUDE_DST)/*.h
