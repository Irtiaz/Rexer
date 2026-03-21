#ifndef NFA_H
#define NFA_H

#include "stb_ds.h"

typedef struct NFA_State_Struct NFA_State;

typedef struct {
	char key;
	NFA_State **value; 
} Transition_Table;

struct NFA_State_Struct {
	Transition_Table *transition;
};

void nfa_add_transition(NFA_State *from, char symbol, NFA_State *to);
void nfa_free_state(NFA_State *state);

#endif
