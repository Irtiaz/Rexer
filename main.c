#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include "NFA.h"

int main(void) {
	NFA *nfa1 = nfa_build_from_symbol("a");
	NFA *nfa2 = nfa_build_from_symbol("b");
	NFA *nfa3 = nfa_build_from_symbol("c");

	nfa_concat(nfa1, nfa2);
	nfa_union(nfa1, nfa3);

	nfa_print(nfa1);

	char testcases[][10] = {
		"a",
		"b",
		"c",
		"ab",
		"ac",
		"bc",
		"abc"
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
