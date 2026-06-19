CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_DEFAULT_SOURCE
TARGET = tock
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

all: $(TARGET)

$(TARGET): tock.c
	$(CC) $(CFLAGS) -o $(TARGET) tock.c

clean:
	rm -f $(TARGET)

install: $(TARGET)
	@echo "Installing to $(BINDIR)..."
	@mkdir -p $(BINDIR)
	@cp $(TARGET) $(BINDIR)/$(TARGET)
	@chmod 755 $(BINDIR)/$(TARGET)
	@echo "Done! You can now run 'tock' from anywhere."

uninstall:
	@echo "Removing from $(BINDIR)..."
	@rm -f $(BINDIR)/$(TARGET)
	@echo "Uninstalled."
