#ifndef NFA_H
#define NFA_H

#include <stdbool.h>
#include "stb_ds.h"

#define NFA_EPSILON "epsilon"

typedef struct NFA_State_Struct NFA_State;

struct NFA_State_Struct {
	struct { char *key; NFA_State **value; } *transition;
};

typedef struct {
	NFA_State *start_state;
	struct { NFA_State *key; bool value; } *accepting_states;
} NFA;


void nfa_build_from_regex(NFA *nfa, const char *regex);
bool nfa_accepts(NFA *nfa, const char *string);
void nfa_free(NFA *nfa, bool owned);

void nfa_print(NFA *nfa);

void nfa_concat(NFA *nfa1, NFA *nfa2); // Modifies the nfa1

#endif
