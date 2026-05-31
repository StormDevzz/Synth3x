# AmnesiaIDE — User Project Makefile
# Compiles persistent projects within Amnesia OS

CC      = gcc
CFLAGS  = -O2 -Wall
TARGET  = main

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c

clean:
	rm -f $(TARGET)
