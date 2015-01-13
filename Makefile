CFLAGS = -std=gnu99 -Wall -Wextra -Wshadow -pedantic -Iinclude -g
CFLAGS += `pkg-config gtk+-3.0 --cflags`

LDFLAGS = -lportaudio -lpthread -lm -lliquid
LDFLAGS += `pkg-config gtk+-3.0 --libs`

SRC = $(wildcard src/lib/*.c)
DEPS = $(SRC:.c=.o)

BIN = salut duplex

all: $(BIN)

salut: src/salut.o $(DEPS)
	@echo "LD $@"
	@gcc $^ -o $@ $(LDFLAGS)

duplex: src/duplex.o $(DEPS)
	@echo "LD $@"
	@gcc $^ -o $@ $(LDFLAGS)

%.o: %.c
	@echo "CC $<"
	@gcc -c $< -o $@ $(CFLAGS)

clean:
	@echo "RM $(DEPS) $(BIN)"
	@rm -f $(DEPS) $(BIN) src/salut.o src/duplex.o

.PHONY: all clean
