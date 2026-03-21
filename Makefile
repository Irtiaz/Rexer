CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -ggdb
SANITIZERS=-fsanitize=address,null,undefined,leak,alignment

main: main.o NFA.o
	$(CC) $^ -o $@ $(SANITIZERS)

main.o: main.c
	$(CC) -c $< -o $@ $(CFLAGS)

NFA.o: NFA.c NFA.h
	$(CC) -c $< -o $@ $(CFLAGS)
