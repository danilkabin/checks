CC = gcc
CFLAGS = -Wall -std=c11 -g
LIBS = -lraylib -lm -lGL -lpthread -ldl -lrt -lX11

TARGET = build/plane_game

SRC = kernel/main.c \
		kernel/window.c \
		kernel/playerModule/player.c \
		kernel/cameraModule/camera.c \
		kernel/planeModule/plane.c \
		engine/hierarchy.c

INCLUDES = -I./include

all: $(TARGET)
	./$(TARGET)

$(TARGET): $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) $(INCLUDES) -o $(TARGET) $(LIBS)

clean:
	rm -rf build
