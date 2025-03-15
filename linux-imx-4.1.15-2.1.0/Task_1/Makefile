CC=gcc
CFLAGS=-Wall -g

TARGET = main
SRCS = main.c add.c del.c
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS)