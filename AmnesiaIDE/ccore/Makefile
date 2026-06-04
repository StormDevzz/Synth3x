CC = gcc
CFLAGS = -O2 -Wall -Wextra -fPIC
AR = ar
OUT = ../build/ccore

all: $(OUT)/libamnesia_core.a $(OUT)/libamnesia_core.so

$(OUT):
	mkdir -p $(OUT)

$(OUT)/core.o: core.c core.h | $(OUT)
	$(CC) $(CFLAGS) -c -o $@ core.c

$(OUT)/libamnesia_core.a: $(OUT)/core.o
	$(AR) rcs $@ $^

$(OUT)/libamnesia_core.so: $(OUT)/core.o
	$(CC) -shared -o $@ $^

clean:
	rm -rf $(OUT)
