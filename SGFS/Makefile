CC = gcc
CFLAGS = -Wall -g
TARGET = sgfs_cli
SRCS = sgfs_cli.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
