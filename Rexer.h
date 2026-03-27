#ifndef REXER_H
#define REXER_H

#include <stddef.h>
#include "NFA.h"

#define REXER_ERROR_TOKEN -444

typedef struct {
	size_t index;
	size_t line;
	size_t column;
} Rexer_Location;

typedef void (*Rexer_Error_Func)(const char *lexeme, Rexer_Location location, void *user_data);

typedef struct {
	const char *regex;
	NFA *nfa;

	int token;
} Rexer_Rule;

typedef struct {
	char *lexeme;
	int token;

	Rexer_Location start;
	Rexer_Location end;
} Rexer_Token;

typedef struct {
	Rexer_Error_Func handler;
	void *user_data;
} Rexer_Error_Handler;

typedef struct {
	Rexer_Rule *rules;
	Rexer_Error_Handler error_handler;

	const char *source;
	size_t start;

	// Following fields will be set automatically when needed
	size_t source_length;
	size_t *line_starts;
	char *source_dup;
} Rexer;

void rexer_set_rule(Rexer *rexer, const char *regex, int token);

void rexer_set_error_handler(Rexer *rexer, Rexer_Error_Func handler, void *user_data);
void rexer_free(Rexer *rexer);

bool rexer_has_next(Rexer *rexer);
Rexer_Token rexer_next(Rexer *rexer);

void rexer_start(Rexer *rexer, const char *string);

#endif
