# AmnesiaIDE — build system
# Rust frontend + C core

CC      = gcc
CFLAGS  = -O2 -Wall -fPIC
CARGO   = cargo
PREFIX  = /usr/local
BINDIR  = $(PREFIX)/bin
ICONDIR = $(PREFIX)/share/icons/hicolor/scalable/apps
APPDIR  = $(PREFIX)/share/applications
CORE    = build/ccore/libamnesia_core.a

all: build/ccore build/release $(CORE) $(BINDIR)/amnesia-ide

$(BINDIR)/amnesia-ide:
	$(CARGO) build --release
	cp target/release/amnesia-ide $(BINDIR)/amnesia-ide
	chmod 755 $(BINDIR)/amnesia-ide

build/ccore build/release:
	mkdir -p $@

$(CORE): ccore/core.c ccore/core.h | build/ccore
	$(CC) $(CFLAGS) -c -o build/ccore/core.o ccore/core.c
	ar rcs $@ build/ccore/core.o

install: all
	install -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(ICONDIR) $(DESTDIR)$(APPDIR)
	install -m 755 target/release/amnesia-ide $(DESTDIR)$(BINDIR)/
	install -m 644 icons/amnesia-ide.png $(DESTDIR)$(ICONDIR)/
	install -m 644 icons/amnesia-ide.desktop $(DESTDIR)$(APPDIR)/

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/amnesia-ide
	rm -f $(DESTDIR)$(ICONDIR)/amnesia-ide.png
	rm -f $(DESTDIR)$(APPDIR)/amnesia-ide.desktop

release: all
	$(CARGO) build --release
	strip target/release/amnesia-ide
	cp target/release/amnesia-ide build/release/

clean:
	$(CARGO) clean
	rm -rf build

.PHONY: all install uninstall release clean
