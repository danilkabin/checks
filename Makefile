CC := gcc
AR := ar
CFLAGS := -Wall -Wextra -O2 -fPIC -Isrc/onion -Isrc/utils/include
SRC_DIRS := src/onion src/utils/src
BUILD_DIR := build
LIB_DIR := build/lib
OBJ_DIR := build/obj
TARGET := $(LIB_DIR)/libonion.a

SRCS := $(shell find $(SRC_DIRS) -name '*.c')
OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
