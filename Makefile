CC := ccache gcc
AR := ar
CFLAGS := -Wall -Wextra -O2 -fPIC -Wno-unused-parameter -Wno-unused-variable -Isrc/onion -Isrc/utils/include -Iinclude
SRC_DIRS := src/onion src/utils/src
BUILD_DIR := build
LIB_DIR := build/lib
OBJ_DIR := build/obj
INCLUDE_DST := include/onion
TARGET := $(LIB_DIR)/libonion.a
TEST_EXE := script

SRCS := $(shell find $(SRC_DIRS) -name '*.c')
OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))
HEADERS := $(shell find src/onion -name '*.h')

.PHONY: all clean headers

all: headers $(TARGET) $(TEST_EXE) copy_config

# Копировать заголовки
headers:
	@mkdir -p $(INCLUDE_DST)
	@cp $(HEADERS) $(INCLUDE_DST)

$(TARGET): $(OBJS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_EXE): tests/request/script.c $(TARGET)
	$(CC) $(CFLAGS) tests/request/script.c -L$(LIB_DIR) -lonion -pthread -o $@

copy_config:
	@mkdir -p $(BUILD_DIR)
	@cp user/config.ini $(BUILD_DIR)/

clean:
	rm -rf $(BUILD_DIR) $(TEST_EXE)
	rm -rf $(INCLUDE_DST)/*.h
