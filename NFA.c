#include "NFA.h"
#include "stb_ds.h"

void nfa_add_transition(NFA_State *from, char symbol, NFA_State *to) {
	NFA_State **to_list = hmget(from->transition, symbol);
	arrput(to_list, to);
	hmput(from->transition, symbol, to_list);
}

void nfa_free_state(NFA_State *state) {
	for (int i = 0; i < hmlen(state->transition); ++i) {
		arrfree(state->transition[i].value);
	}
	hmfree(state->transition);
}
