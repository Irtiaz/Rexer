#ifndef NFA_H
#define NFA_H

#include <stdbool.h>
#include "stb_ds.h"

#define NFA_EPSILON "epsilon"
#define NFA_ANY "any"

typedef struct NFA_State_Struct NFA_State;

struct NFA_State_Struct {
	struct { char *key; NFA_State **value; } *transition;
};

typedef struct {
	NFA_State *start_state;
	struct { NFA_State *key; bool value; } *accepting_states;
} NFA;


NFA *nfa_build_from_symbol(const char *symbol);
NFA *nfa_build_from_regex(const char *regex);
bool nfa_accepts(NFA *nfa, const char *string);
void nfa_free(NFA *nfa, bool owned);

NFA *nfa_deep_copy(NFA *nfa);
void nfa_print(NFA *nfa);

void nfa_concat(NFA *nfa1, NFA *nfa2); // Modifies nfa1
void nfa_union(NFA *nfa1, NFA *nfa2); // Modifies nfa1
void nfa_kleene_star(NFA *nfa); // Modifies nfa
void nfa_kleene_plus(NFA *nfa); // Modifies nfa

// TODO remove later
void print_tokens(const char *regex);

#endif
