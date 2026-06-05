CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE $(shell pkg-config --cflags gtk+-3.0)
LDLIBS  = $(shell pkg-config --libs gtk+-3.0)

SRCS    = src/main.c src/nav/browse.c src/ops/fileops.c src/ops/extras.c src/anim/anim.c src/checks/terminal.c
OBJS    = $(SRCS:.c=.o)
TARGET  = fileman

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

src/%.o: src/%.c src/fm.h
	$(CC) $(CFLAGS) -c -o $@ $<

src/nav/%.o: src/nav/%.c src/fm.h
	$(CC) $(CFLAGS) -c -o $@ $<

src/ops/%.o: src/ops/%.c src/fm.h
	$(CC) $(CFLAGS) -c -o $@ $<

src/anim/%.o: src/anim/%.c src/fm.h
	$(CC) $(CFLAGS) -c -o $@ $<

src/checks/%.o: src/checks/%.c src/fm.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
