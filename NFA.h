#ifndef NFA_H
#define NFA_H

#include <stdbool.h>
#include "stb_ds.h"

typedef struct NFA_State_Struct NFA_State;

struct NFA_State_Struct {
	struct { char *key; NFA_State **value; } *transition;
};

typedef struct {
	NFA_State **states;
	NFA_State *start_state;
	struct { NFA_State *key; bool value; } *accepting_states;
} NFA;

void nfa_state_init(NFA_State *state);
void nfa_add_transition(NFA_State *from, const char *symbol, NFA_State *to);
void nfa_state_free(NFA_State *state);

void nfa_build_from_regex(NFA *nfa, const char *regex);
bool nfa_accepts(NFA *nfa, const char *string);
void nfa_free(NFA *nfa);

#endif
