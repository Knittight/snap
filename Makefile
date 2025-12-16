CC      = gcc
CFLAGS  = -Wall -Wextra -O2
SRC     = snap.c
BIN     = src/snap

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

clean:
	rm -f $(BIN)

.PHONY: all clean
