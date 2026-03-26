#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include "NFA.h"

int main(void) {
	NFA *nfa = nfa_build_from_regex("a|b|a.c");

	nfa_print(nfa);

	char testcases[][100] = {
		"",
		"a",
		"b",
		"c",
		"a.c",
		"abc",
		"aac",
		"abcab"
	};

	size_t num_testcases = sizeof(testcases) / sizeof(testcases[0]);
	for (size_t i = 0; i < num_testcases; ++i) {
		printf("Accepts \"%s\": %d\n", testcases[i], nfa_accepts(nfa, testcases[i]));
	}

	nfa_free(nfa, true);
	return 0;
}

int main2(void) {
	NFA *nfa1 = nfa_build_from_symbol("a");
	NFA *nfa2 = nfa_build_from_symbol(NFA_ANY);
	NFA *nfa3 = nfa_build_from_symbol("c");

	nfa_concat(nfa1, nfa2);
	nfa_union(nfa1, nfa3);
	nfa_kleene_plus(nfa1);

	nfa_print(nfa1);

	char testcases[][100] = {
		"",
		"a",
		"aaaa",
		"b",
		"c",
		"ccc",
		"ab",
		"ac",
		"bc",
		"abc",
		"ababab",
		"ababccccab",
		"ababcccca",
		"acacac",
		"aca"
	};

	size_t num_testcases = sizeof(testcases) / sizeof(testcases[0]);
	for (size_t i = 0; i < num_testcases; ++i) {
		printf("Accepts \"%s\": %d\n", testcases[i], nfa_accepts(nfa1, testcases[i]));
	}

	nfa_free(nfa1, true);
	nfa_free(nfa2, false);
	nfa_free(nfa3, false);
	return 0;
}
