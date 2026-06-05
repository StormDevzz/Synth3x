CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE $(shell pkg-config --cflags gtk+-3.0)
LDLIBS  = $(shell pkg-config --libs gtk+-3.0)

SRCS    = src/main.c src/filelist.c src/fileops.c
OBJS    = $(SRCS:.c=.o)
TARGET  = fileman

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

src/%.o: src/%.c src/fm.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
