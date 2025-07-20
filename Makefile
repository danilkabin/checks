CC := ccache gcc
AR := ar
CFLAGS := -Wall -Wextra -O2 -fPIC -Iinclude -Wno-unused-parameter -Wno-unused-variable -Isrc -Isrc/misc
SRC_DIRS := src src/utils
BUILD_DIR := build
LIB_DIR := build/lib
OBJ_DIR := build/obj
INCLUDE_DST := include/uidq
TARGET := $(LIB_DIR)/libuidq.a
TEST_EXE := script
TARGET_SCRIPT = examples/bitmask.c

SRCS := $(shell find $(SRC_DIRS) -name '*.c')
OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))
HEADERS := $(shell find src -name '*.h')

.PHONY: all clean headers

all: headers $(TARGET) $(TEST_EXE) copy_config

# Копировать заголовки
headers:
	@mkdir -p $(INCLUDE_DST)
	@rsync -a --include '*/' --include '*.h' --exclude '*' src $(INCLUDE_DST)/

$(TARGET): $(OBJS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_EXE): $(TARGET_SCRIPT) $(TARGET)
	$(CC) $(CFLAGS) $(TARGET_SCRIPT) -L$(LIB_DIR) -luidq -pthread -o $@

copy_config:
	@mkdir -p $(BUILD_DIR)
	@cp config/config.ini $(BUILD_DIR)/

clean:
	rm -rf $(BUILD_DIR) $(TEST_EXE)
	rm -rf $(INCLUDE_DST)/*.h
