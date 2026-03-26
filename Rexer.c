#include "Rexer.h"

void rexer_register_rule(Rexer *rexer, const char *regex, rexer_rule_handler handler, void *user_data) {
	arrput(rexer->rules, ((Rexer_Rule){
				.regex = strdup(regex),
				.handler = handler,
				.nfa = nfa_from_regex(regex),
				.user_data = user_data
	}));
}

void rexer_free(Rexer *rexer) {
	for (int i = 0; i < arrlen(rexer->rules); ++i) {
		Rexer_Rule rule = rexer->rules[i];
		free((void *)rule.regex);
		nfa_free(rule.nfa);
	}

	arrfree(rexer->rules);
}
