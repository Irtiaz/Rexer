#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include "NFA.h"

int main(void) {
	NFA_State state0 = {0};
	NFA_State state1 = {0};

	nfa_add_transition(&state0, 'a', &state1);

	nfa_free_state(&state0);
	nfa_free_state(&state1);

	return 0;
}
