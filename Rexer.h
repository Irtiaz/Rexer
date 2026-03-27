#ifndef REXER_H
#define REXER_H

#include <stddef.h>
#include "NFA.h"

typedef struct {
	size_t index;
	size_t line;
	size_t column;
} Rexer_Location;

typedef void (*Rexer_Handler)(const char *lexeme, Rexer_Location start, Rexer_Location end, void *user_data);

typedef struct {
	const char *regex;
	Rexer_Handler handler;
	NFA *nfa;

	void *user_data;
} Rexer_Rule;

typedef struct {
	Rexer_Rule *rules;
	Rexer_Rule error_handler;
} Rexer;

void rexer_register_rule(Rexer *rexer, const char *regex, Rexer_Handler handler, void *user_data);
void rexer_register_error_handler(Rexer *rexer, Rexer_Handler handler, void *user_data);
void rexer_free(Rexer *rexer);

void rexer_start(Rexer *rexer, const char *string);

#endif
