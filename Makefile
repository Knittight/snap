CC      = gcc
CFLAGS  = -Wall -Wextra -O2
SRC     = src/snap.c
BIN     = snap

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

make: $(BIN)
	gcc -Wall -Wextra -O2 src/snap.c -o snap
	cp snap /usr/local/bin
	rm snap
	echo "snap is installed"

clean:
	rm -f $(BIN)

.PHONY: all clean make
