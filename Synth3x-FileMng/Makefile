CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99 -I include
LDLIBS  = -lncurses

SRCS    = src/core/main.c src/ui/render.c src/ui/dialog.c src/ui/viewer.c \
          src/fs/browse.c src/fs/ops.c
OBJS    = $(SRCS:.c=.o)
TARGET  = fileman

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

src/core/%.o: src/core/%.c include/fileman.h
	$(CC) $(CFLAGS) -c -o $@ $<

src/ui/%.o: src/ui/%.c include/fileman.h
	$(CC) $(CFLAGS) -c -o $@ $<

src/fs/%.o: src/fs/%.c include/fileman.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
