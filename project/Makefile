# Compiler and tools
CC      := gcc
AR      := ar

# Flags - include paths for current structure
CFLAGS  := -Wall -Wextra -O3 -fPIC -march=native \
           -I. -I../include -Iinclude \
           -pthread \
           -Wno-unused-function -Wno-unused-variable

# Directories
CORE_DIR     := core
OS_DIR       := os/unix
APP_DIR      := app
INC_DIR      := ../include

NOISE_DIR    := $(CORE_DIR)/noise

OBJ_DIR      := ../build/obj
BIN_DIR      := ../build/bin
LIB_DIR      := ../build/lib

# Targets
TARGET       := $(BIN_DIR)/project
TARGET_LIB   := $(LIB_DIR)/libopium.a

# Source files
CORE_SRCS    := $(wildcard $(CORE_DIR)/*.c $(NOISE_DIR)/*.c)
OS_SRCS      := $(wildcard $(OS_DIR)/*.c)
APP_SRCS     := $(wildcard $(APP_DIR)/*.c)

SRCS         := $(CORE_SRCS) $(OS_SRCS) $(APP_SRCS)
OBJS         := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))

# Main application file (entry point)
MAIN_SRC     := $(APP_DIR)/opium_main.c

.PHONY: all clean run debug test lib

all: $(TARGET)

# Static library target (core only)
lib: $(TARGET_LIB)

$(TARGET_LIB): $(OBJS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^
	@echo "Static library built: $(TARGET_LIB)"

# Main executable
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS) -lm -o $@
	@echo "Executable built: $(TARGET) (port 8080 → 4308)"

# Compile object files
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Commands
run: all
	@echo "Running $(TARGET)..."
	cd .. && ./build/bin/opium

debug: CFLAGS += -g -O0 -DDEBUG
debug: clean all

test: all
	@echo "Running tests..."

clean:
	rm -rf ../build
	@echo "Build directory cleaned"

# Generate compile_commands.json
bear: clean
	bear -- make

# Show project structure
tree:
	@tree -I 'build|*.o|*.a' --dirsfirst

# Help
help:
	@echo "Available targets:"
	@echo "  all     - Build everything (default)"
	@echo "  lib     - Build static library only"
	@echo "  run     - Build and run the executable"
	@echo "  debug   - Build with debug symbols"
	@echo "  test    - Run tests"
	@echo "  clean   - Remove build files"
	@echo "  bear    - Generate compile_commands.json"
	@echo "  tree    - Show project structure"
