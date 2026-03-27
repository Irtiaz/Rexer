#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include "Rexer.h"

void a_handler(Rexer_Location start, Rexer_Location end, void *user_data) {
	(void)user_data;
	puts("a handler");
	printf("line: %lu, column: %lu\n", start.line, start.column);
	printf("line: %lu, column: %lu\n", end.line, end.column);
}

void b_handler(Rexer_Location start, Rexer_Location end, void *user_data) {
	(void)user_data;
	puts("b handler");
	printf("line: %lu, column: %lu\n", start.line, start.column);
	printf("line: %lu, column: %lu\n", end.line, end.column);
}

void error_handler(Rexer_Location start, Rexer_Location end, void *user_data) {
	(void)user_data;
	puts("Error handler");
	printf("line: %lu, column: %lu\n", start.line, start.column);
	printf("line: %lu, column: %lu\n", end.line, end.column);
}

int main(void) {
	Rexer rexer = {0};

	const char *source = "aaa\naaa";

	rexer_register_rule(&rexer, "a+", a_handler, NULL);
	rexer_register_rule(&rexer, "aaa", b_handler, NULL);
	rexer_register_error_handler(&rexer, error_handler, NULL);

	rexer_start(&rexer, source);

	rexer_free(&rexer);
	return 0;
}

int main2(void) {
	NFA *nfa = nfa_from_regex("a**");
	nfa_print(nfa);

	char testcases[][100] = {
		"",
		"a",
		"aaaa",
		"bbbb",
		"b",
		"c",
		"ccc",
		"ab",
		"ac",
		"bc",
		"abc",
		"a.c",
		"acc",
		"ababab",
		"ababccccab",
		"ababcccca",
		"acacac",
		"aca"
	};

	size_t num_testcases = sizeof(testcases) / sizeof(testcases[0]);
	for (size_t i = 0; i < num_testcases; ++i) {
		bool accepts = nfa_accepts(nfa, testcases[i]);
		if (accepts) printf("\033[0;32m");
		else printf("\033[0;31m");

		printf("\"%s\"\n", testcases[i]);

		printf("\033[0m");
	}

	nfa_free(nfa);

	return 0;
}
