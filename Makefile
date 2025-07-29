CC := ccache gcc
AR := ar
CFLAGS := -Wall -Wextra -O2 -fPIC -Iinclude -Wno-unused-parameter -Wno-unused-variable \
          -Isrc -Isrc/core -Isrc/master -Isrc/misc -Isrc/process

SRC_DIRS := src/core src/master src/misc
PROCESS_DIR := src/process
BUILD_DIR := build
LIB_DIR := $(BUILD_DIR)/lib
OBJ_DIR := $(BUILD_DIR)/obj
PROCESS_OBJ_DIR := $(BUILD_DIR)/process/obj
BIN_DIR := $(BUILD_DIR)/bin
INCLUDE_DST := include/uidq

TARGET_LIB := $(LIB_DIR)/libuidq.a
TARGET_MASTER := $(BIN_DIR)/script
TARGET_PROCESS := $(BIN_DIR)/process_worker
TARGET_SCRIPT_SRC := examples/main.c

# Общие исходники (для библиотеки)
SRCS := $(shell find $(SRC_DIRS) -name '*.c')
OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))

# Исходники и объектники для process_worker
PROCESS_SRCS := $(wildcard $(PROCESS_DIR)/*.c)
PROCESS_OBJS := $(patsubst $(PROCESS_DIR)/%.c,$(PROCESS_OBJ_DIR)/%.o,$(PROCESS_SRCS))

.PHONY: all clean headers

all: headers $(TARGET_LIB) $(TARGET_MASTER) $(TARGET_PROCESS)

headers:
	@mkdir -p $(INCLUDE_DST)
	@rsync -a --include '*/' --include '*.h' --exclude '*' src $(INCLUDE_DST)/

$(TARGET_LIB): $(OBJS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_MASTER): $(TARGET_SCRIPT_SRC) $(TARGET_LIB)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(TARGET_SCRIPT_SRC) -L$(LIB_DIR) -luidq -pthread -o $@

$(TARGET_PROCESS): $(PROCESS_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(PROCESS_OBJ_DIR)/%.o: $(PROCESS_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(INCLUDE_DST)/*.h
