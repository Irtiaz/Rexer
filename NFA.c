#include "NFA.h"
#include "stb_ds.h"

void nfa_add_transition(NFA_State *from, const char *symbol, NFA_State *to) {
	NFA_State **to_list = shget(from->transition, symbol);
	arrput(to_list, to);
	shput(from->transition, symbol, to_list);
}

void nfa_free_state(NFA_State *state) {
	for (int i = 0; i < hmlen(state->transition); ++i) {
		arrfree(state->transition[i].value);
	}
	hmfree(state->transition);
}
