# AmnesiaIDE — GUI build system
# Pure Rust (eframe/egui)

CARGO   = cargo
PREFIX  = /usr/local
BINDIR  = $(PREFIX)/bin
ICONDIR = $(PREFIX)/share/icons/hicolor/scalable/apps
APPDIR  = $(PREFIX)/share/applications

all: release

release: build/amnesia-ide

build/amnesia-ide: src/main.rs src/editor.rs src/syntax.rs
	$(CARGO) build --release
	mkdir -p build
	cp target/release/amnesia-ide build/amnesia-ide

install: all
	install -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(ICONDIR) $(DESTDIR)$(APPDIR)
	install -m 755 build/amnesia-ide $(DESTDIR)$(BINDIR)/
	install -m 644 icons/amnesia-ide.png $(DESTDIR)$(ICONDIR)/
	install -m 644 icons/amnesia-ide.desktop $(DESTDIR)$(APPDIR)/

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/amnesia-ide
	rm -f $(DESTDIR)$(ICONDIR)/amnesia-ide.png
	rm -f $(DESTDIR)$(APPDIR)/amnesia-ide.desktop

clean:
	$(CARGO) clean
	rm -rf build

.PHONY: all release install uninstall clean
