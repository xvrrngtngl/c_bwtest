CC     = gcc
CFLAGS = -Wall -Wextra
LDLIBS = -lcurl

BIN = c_bwtest
SRC = src/main.c src/common.c src/cjson/cJSON.c

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@ $(LDLIBS)

clean:
	rm -f $(BIN)

.PHONY: clean
