CC := ccache gcc
AR := ar
CFLAGS := -Wall -Wextra -O2 -fPIC -Iinclude -Wno-unused-parameter -Wno-unused-variable -Isrc -Isrc/core -Isrc/master -Isrc/misc

SRC_DIRS := src/core src/master src/misc
BUILD_DIR := build
LIB_DIR := $(BUILD_DIR)/lib
OBJ_DIR := $(BUILD_DIR)/obj
INCLUDE_DST := include/uidq

TARGET_LIB := $(LIB_DIR)/libuidq.a
TARGET_MASTER := $(BUILD_DIR)/bin/script
TARGET_SCRIPT_SRC := examples/main.c

SRCS := $(shell find $(SRC_DIRS) -name '*.c')
OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean headers

all: headers $(TARGET_LIB) $(TARGET_MASTER)

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
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(TARGET_SCRIPT_SRC) -L$(LIB_DIR) -luidq -pthread -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(INCLUDE_DST)/*.h
