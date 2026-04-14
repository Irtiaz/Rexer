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

	// Following fields will be set automatically when needed
	NFA_State **current_states;
} NFA;


NFA *nfa_from_regex(const char *regex);
bool nfa_accepts(NFA *nfa, const char *string);
void nfa_free(NFA *nfa, bool owner);
void nfa_rewind(NFA *nfa);
bool nfa_forward(NFA *nfa, char c);
bool nfa_is_alive(NFA *nfa);

void nfa_print(NFA *nfa);

void nfa_concat(NFA *nfa1, NFA *nfa2); // Modifies nfa1
void nfa_union(NFA *nfa1, NFA *nfa2); // Modifies nfa1
NFA *nfa_union_all(NFA **nfas);
void nfa_optional(NFA *nfa); // Modifies nfa
void nfa_kleene_star(NFA *nfa); // Modifies nfa
void nfa_kleene_plus(NFA *nfa); // Modifies nfa

#endif
