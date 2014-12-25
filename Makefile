CFLAGS = -std=gnu99 -Wall -Wextra -Wshadow -pedantic -Iinclude -g
LDFLAGS = -lportaudio -lpthread -lm

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

BIN = salut

all: $(BIN)

$(BIN): $(OBJ)
	@echo "LD $@"
	@gcc $^ -o $@ $(LDFLAGS)

%.o: %.c
	@echo "CC $<"
	@gcc -c $< -o $@ $(CFLAGS)

clean:
	@echo "RM $(wildcard *.o) $(BIN)"
	@rm -f *.o $(BIN)

.PHONY: all clean
