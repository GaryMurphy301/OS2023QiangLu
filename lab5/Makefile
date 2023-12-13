# Makefile for Thread Pool Web Server

CC = gcc
CFLAGS = -Wall -pthread -DNDEBUG


# List of source files
SRC_FILES = threadpool.c webserver.c

# Generate corresponding object file names
OBJ_FILES = $(SRC_FILES:.c=.o)

# Target executable name
TARGET = threadpool

# Rule to build the executable
$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ_FILES)

# Rule to build object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean the project
clean:
	rm -f $(OBJ_FILES) $(TARGET)

