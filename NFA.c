#include "NFA.h"
#include "stb_ds.h"

void nfa_state_init(NFA_State *state) {
	state->transition = NULL;
	sh_new_arena(state->transition);
}

void nfa_add_transition(NFA_State *from, const char *symbol, NFA_State *to) {
	NFA_State **to_list = shget(from->transition, symbol);
	arrput(to_list, to);
	shput(from->transition, symbol, to_list);
}

void nfa_state_free(NFA_State *state) {
	for (int i = 0; i < hmlen(state->transition); ++i) {
		arrfree(state->transition[i].value);
	}
	hmfree(state->transition);
}

void nfa_build_from_regex(NFA *nfa, const char *regex) {
	NFA_State *state0 = malloc(sizeof(NFA_State));
	nfa_state_init(state0);

	NFA_State *state1 = malloc(sizeof(NFA_State));
	nfa_state_init(state1);

	nfa_add_transition(state0, regex, state1);

	arrput(nfa->states, state0);
	arrput(nfa->states, state1);

	nfa->start_state = state0;
	hmput(nfa->accepting_states, state1, true);
}

static NFA_State **state_next(NFA_State *state, const char *symbol) {
	NFA_State **nexts = shget(state->transition, symbol);
	if (arrlen(nexts) != 0) return nexts;
	return NULL;
}

static NFA_State **state_list_next(NFA_State **state_list, const char *symbol) {
	NFA_State **result = NULL;

	for (int i = 0; i < arrlen(state_list); ++i) {
		NFA_State **nexts = state_next(state_list[i], symbol);

		for (int j = 0; j < arrlen(nexts); ++j) {
			arrput(result, nexts[j]);
		}

	}
	return result;
}

bool nfa_accepts(NFA *nfa, const char *string) {
	NFA_State **current_states = NULL;
	arrput(current_states, nfa->start_state);

	while (*string != '\0' && arrlen(current_states) != 0) {
		char symbol[2];
		symbol[0] = *string;
		symbol[1] = '\0';

		NFA_State **next_states = state_list_next(current_states, symbol);
		arrfree(current_states);
		current_states = next_states;

		++string;
	}

	if (current_states == NULL) return false;

	for (int i = 0; i < arrlen(current_states); ++i) {
		if (hmget(nfa->accepting_states, current_states[i])) return true;
	}

	arrfree(current_states);

	return false;
}

void nfa_free(NFA *nfa) {
	for (int i = 0; i < arrlen(nfa->states); ++i) {
		nfa_state_free(nfa->states[i]);
		free(nfa->states[i]);
	}
	arrfree(nfa->states);

	hmfree(nfa->accepting_states);
}
