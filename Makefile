# Copyright (c) 2014 - 2015 Jean Bégaint

# This file is part of salut.

# salut is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# salut is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with salut.  If not, see <http://www.gnu.org/licenses/>.

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
