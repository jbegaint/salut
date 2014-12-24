CFLAGS = -std=c99 -Wall -Wextra -Wshadow -pedantic -g
LDFLAGS = -lportaudio -lpthread

SRC = test_record_playback.c \
	  test_rt_playback.c

OBJ = $(SRC:.c=.o)
BIN = $(SRC:.c=)

all: $(BIN)

$(OBJ):

.o:
	@echo "LD $@"
	@gcc $< -o $@ $(LDFLAGS)

%.o: %.c
	@echo "CC $<"
	@gcc -c $< -o $@ $(CFLAGS)

clean:
	@echo "RM $(wildcard *.o) $(BIN)"
	@rm -f *.o $(BIN)

.PHONY: all clean
