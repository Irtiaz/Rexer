#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include "NFA.h"

int main(void) {
	NFA nfa = {0};
	nfa_build_from_regex(&nfa, "a");

	printf("Accepts: %d\n", nfa_accepts(&nfa, "a"));
	printf("Accepts: %d\n", nfa_accepts(&nfa, "b"));

	nfa_free(&nfa);
	return 0;
}
