CC      = gcc
CFLAGS  = -Wall -Wextra -O2
SRC     = chrono.c
BIN     = src/chrono

all: $(BIN)

$(BIN): $(SRC)
	mkdir -p src
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

clean:
	rm -f $(BIN)

.PHONY: all clean