CC      = gcc
CFLAGS  = -Wall -Wextra -O2
SRC     = src/snap.c
BIN     = snap

all: $(BIN) install

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

install: $(BIN)
	sudo cp $(BIN) /usr/local/bin/
	@echo "snap installed to /usr/local/bin"

clean:
	rm -f $(BIN)

.PHONY: all install clean