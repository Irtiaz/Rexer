#ifndef REXER_H
#define REXER_H

#include <stddef.h>
#include "NFA.h"

typedef struct {
	size_t line;
	size_t column;
} Rexer_Location;

typedef void (*rexer_rule_handler)(Rexer_Location location, void *user_data);

typedef struct {
	const char *regex;
	rexer_rule_handler handler;
	NFA *nfa;

	void *user_data;
} Rexer_Rule;

typedef struct {
	Rexer_Rule *rules;
} Rexer;

void rexer_register_rule(Rexer *rexer, const char *regex, rexer_rule_handler handler, void *user_data);
void rexer_free(Rexer *rexer);

#endif
