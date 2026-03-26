#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include "Rexer.h"

void handler(Rexer_Location location, void *user_data) {
	(void)user_data;
	printf("line: %lu, column: %lu\n", location.line, location.column);
}

int main(void) {
	Rexer rexer = {0};

	rexer_register_rule(&rexer, ".*", handler, NULL);

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
