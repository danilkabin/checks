CC := ccache gcc
AR := ar
CFLAGS := -Wall -Wextra -O2 -fPIC -Iinclude -Wno-unused-parameter -Wno-unused-variable \
          -Isrc -Isrc/core -Isrc/master -Isrc/misc -Isrc/process

SRC_DIRS := src/core src/master src/misc src/unix
PROCESS_DIR := src/process
BUILD_DIR := build

STATIC_CONFIG := config/config.ini

UIDQ_H := src/core/uidq.h

VERSION := $(shell \
	grep '#define UIDQ_VERSION' $(UIDQ_H) | \
	awk '{print $$3}' | tr -d '"')

VERSION_DIR := build/$(VERSION)

LIB_DIR := $(VERSION_DIR)/lib
OBJ_DIR := $(VERSION_DIR)/obj
PROCESS_OBJ_DIR := $(VERSION_DIR)/process/obj
BIN_DIR := $(VERSION_DIR)/bin
INCLUDE_DST := include/uidq

TARGET_LIB := $(LIB_DIR)/libuidq.a
TARGET_MASTER := $(BIN_DIR)/uidq
TARGET_PROCESS := $(BIN_DIR)/process_worker

SRCS := $(shell find $(SRC_DIRS) -name '*.c')
OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))

PROCESS_SRCS := $(wildcard $(PROCESS_DIR)/*.c)
PROCESS_OBJS := $(patsubst $(PROCESS_DIR)/%.c,$(PROCESS_OBJ_DIR)/%.o,$(PROCESS_SRCS))

.PHONY: all clean headers

all: $(VERSION_DIR) headers $(TARGET_LIB) $(TARGET_MASTER) $(TARGET_PROCESS)

$(VERSION_DIR):
	@mkdir -p $(VERSION_DIR)
	@cp $(STATIC_CONFIG) $(VERSION_DIR) 
	@echo "Created folder: $(VERSION_DIR)"

headers:
	@mkdir -p $(INCLUDE_DST)
	@rsync -a --include '*/' --include '*.h' --exclude '*' src $(INCLUDE_DST)/

$(TARGET_LIB): $(OBJS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

MASTER_SRCS := src/core/uidq.c
$(TARGET_MASTER): $(MASTER_SRCS) $(TARGET_LIB)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -L$(LIB_DIR) -luidq -pthread -o $@

$(TARGET_PROCESS): $(PROCESS_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(PROCESS_OBJ_DIR)/%.o: $(PROCESS_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(VERSION_DIR)
	rm -rf $(INCLUDE_DST)/*.h
