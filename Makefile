CFLAGS = -std=gnu99 -Wall -Wextra -Wshadow -pedantic -Iinclude -g
CFLAGS += `pkg-config gtk+-3.0 --cflags`

LDFLAGS = -lportaudio -lpthread -lm -lliquid
LDFLAGS += `pkg-config gtk+-3.0 --libs`

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

BIN = salut

# DEV ONLY
all:
	make clean && make $(BIN) -j4

# all: $(BIN)

$(BIN): $(OBJ)
	@echo "LD $@"
	@gcc $^ -o $@ $(LDFLAGS)

%.o: %.c
	@echo "CC $<"
	@gcc -c $< -o $@ $(CFLAGS)

clean:
	@echo "RM $(OBJ) $(BIN)"
	@rm -f $(OBJ) $(BIN)

.PHONY: all clean
