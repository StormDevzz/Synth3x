CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99
LDLIBS  = -lncurses

SRCS    = main.c ui.c browse.c
OBJS    = $(SRCS:.c=.o)
TARGET  = fileman

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c fileman.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
