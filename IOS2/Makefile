CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic -pthread
FILES= proj2.c proj2.h

.PHONY: proj2 clean

all:proj2

proj2: $(FILES)
	$(CC) $(CFLAGS) -o $@ $(FILES)

clean:
	@rm -f proj2