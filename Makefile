CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -ggdb
SANITIZERS=-fsanitize=address,null,undefined,leak,alignment

.PHONY: all
all: handler_example next_example

handler_example: handler_example.o NFA.o Rexer.o
	$(CC) $^ -o $@ #$(SANITIZERS)

handler_example.o: handler_example.c NFA.h Rexer.h
	$(CC) -c $< -o $@ $(CFLAGS)

next_example: next_example.o NFA.o Rexer.o
	$(CC) $^ -o $@ #$(SANITIZERS)

next_example.o: next_example.c NFA.h Rexer.h
	$(CC) -c $< -o $@ $(CFLAGS)

NFA.o: NFA.c NFA.h
	$(CC) -c $< -o $@ $(CFLAGS)

Rexer.o: Rexer.c Rexer.h NFA.h
	$(CC) -c $< -o $@ $(CFLAGS)
