CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -ggdb -Wswitch-enum
SANITIZERS=-fsanitize=address,null,undefined,leak,alignment

main: main.o NFA.o Rexer.o
	$(CC) $^ -o $@ #$(SANITIZERS)

main.o: main.c NFA.h Rexer.h
	$(CC) -c $< -o $@ $(CFLAGS)

NFA.o: NFA.c NFA.h
	$(CC) -c $< -o $@ $(CFLAGS)

Rexer.o: Rexer.c Rexer.h NFA.h
	$(CC) -c $< -o $@ $(CFLAGS)
