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

static void remove_duplicates(NFA_State ***p_list) {
  struct {
    NFA_State *key;
    bool value;
  } *map = NULL;

  for (int i = 0; i < arrlen(*p_list); ++i) {
    hmput(map, (*p_list)[i], true);
  }

  for (int i = 0; i < hmlen(map); ++i) {
    NFA_State *state = map[i].key;
    (*p_list)[i] = state;
  }

  while (arrlen(*p_list) > hmlen(map)) {
    arrdel(*p_list, hmlen(map));
  }

  hmfree(map);
}

static void merge_state_list(NFA_State ***p_list, NFA_State **other) {
  for (int i = 0; i < arrlen(other); ++i) {
    arrput(*p_list, other[i]);
  }

  remove_duplicates(p_list);
}

static NFA_State **state_next(NFA_State *state, const char *symbol) {
  NFA_State **nexts = shget(state->transition, symbol);

  NFA_State **empty_reachables = NULL;
  for (int i = 0; i < arrlen(nexts); ++i) {
    NFA_State **epsilons = state_next(nexts[i], NFA_EPSILON);
    merge_state_list(&empty_reachables, epsilons);
  }
  merge_state_list(&nexts, empty_reachables);

  return nexts;
}

static NFA_State **state_list_next(NFA_State **state_list, const char *symbol) {
  NFA_State **result = NULL;

  for (int i = 0; i < arrlen(state_list); ++i) {
    NFA_State **nexts = state_next(state_list[i], symbol);
    merge_state_list(&result, nexts);
  }

  return result;
}

bool nfa_accepts(NFA *nfa, const char *string) {
  NFA_State **current_states = NULL;
  arrput(current_states, nfa->start_state);

  while (*string != '\0' && arrlen(current_states) != 0) {
    char symbol[2] = {0};
    symbol[0] = *string;

    NFA_State **next_states = state_list_next(current_states, symbol);
    arrfree(current_states);
    current_states = next_states;

    ++string;
  }

	bool result = false;

  if (current_states == NULL) {
		result = false;
		goto nfa_accepts_defer;
	}

  for (int i = 0; i < arrlen(current_states); ++i) {
    if (hmget(nfa->accepting_states, current_states[i])) {
			result = true;
			goto nfa_accepts_defer;
    }
  }

nfa_accepts_defer:
  arrfree(current_states);
  return result;
}

void nfa_free(NFA *nfa) {
  for (int i = 0; i < arrlen(nfa->states); ++i) {
    nfa_state_free(nfa->states[i]);
    free(nfa->states[i]);
  }
  arrfree(nfa->states);

  hmfree(nfa->accepting_states);
}

/* void nfa_concat(NFA *nfa1, NFA *nfa2) { */
/* 	NFA_State **to_be_removed = NULL; */

/* 	for (int i = 0; i < hmlen(nfa1->accepting_states); ++i) { */
/* 		NFA_State *accepting_state = nfa1->accepting_states[i].key; */
/* 		nfa_add_transition(accepting_state, NFA_EPSILON,
 * nfa2->start_state); */

/* 		arrput(to_be_removed, accepting_state); */
/* 	} */

/* 	for (int i = 0; i < arrlen(to_be_removed); ++i) { */
/* 		hmdel(nfa1->accepting_states, to_be_removed[i]); */
/* 	} */
/* } */
