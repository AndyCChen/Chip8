# files to compile
OBJS = main.c src/chip8.c src/display.c

# compiler to use
CC = gcc

# includes paths
INCLUDE_PATHS = -IC:\chip8_lib\SDL\include\SDL2

# library paths
LIBRARY_PATHS = -LC:\chip8_lib\SDL\lib

# compilier flags
COMPILIER_FLAGS = -w

# linker flags
LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2

# name of executable
OBJ_NAME = chip8

all:
	$(CC) $(OBJS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)