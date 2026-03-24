#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include "NFA.h"

int main(void) {
	NFA nfa1 = {0};
	NFA nfa2 = {0};

	nfa_build_from_regex(&nfa1, "a");
	nfa_build_from_regex(&nfa2, "b");

	nfa_concat(&nfa1, &nfa2);

	nfa_print(&nfa1);

	printf("Accepts \"a\": %d\n", nfa_accepts(&nfa1, "a"));
	printf("Accepts \"ab\": %d\n", nfa_accepts(&nfa1, "ab"));

	nfa_free(&nfa1, true);
	nfa_free(&nfa2, false);
	return 0;
}
