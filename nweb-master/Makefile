# Makefile for webserver.c

CC = gcc
CFLAGS = -Wall
TARGET = nweb
SOURCE = nweb23.c
OBJS = $(SOURCE:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(OBJS): $(SOURCE)
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)

