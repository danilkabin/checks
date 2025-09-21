CC := gcc
AR := ar
CFLAGS := -Wall -Wextra -O2 -fPIC -Iinclude/opium -pthread

SRC_DIRS := . core platform/unix uvent
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
LIB_DIR := $(BUILD_DIR)/lib
BIN_DIR := $(BUILD_DIR)/bin
INCLUDE_DIR := include/opium

TARGET_LIB := $(LIB_DIR)/libuidq.a
TARGET_MASTER := $(BIN_DIR)/opium

SRCS := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean headers

all: headers $(TARGET_LIB) $(TARGET_MASTER)

headers:
	@mkdir -p $(INCLUDE_DIR)
	@cp -r core $(INCLUDE_DIR)/ || true
	@cp -r platform $(INCLUDE_DIR)/ || true
	@cp -r uvent $(INCLUDE_DIR)/ || true

$(TARGET_LIB): $(OBJS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_MASTER): $(TARGET_LIB) opium.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) opium.c -L$(LIB_DIR) -luidq -lm -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(INCLUDE_DIR)
