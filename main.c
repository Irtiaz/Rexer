#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include "NFA.h"

int main(void) {
	NFA *nfa = nfa_from_regex("(ab|c)+");
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

	nfa_free(nfa, true);
}
