#include "NFA.h"
#include <assert.h>
#include <stdio.h>
#include "stb_ds.h"

typedef struct {
	NFA_State *key;
	bool value;
} Traversal_Map;

static NFA_State *nfa_state_init(void) {
	NFA_State *state = malloc(sizeof(NFA_State));
  state->transition = NULL;
  sh_new_arena(state->transition);

	return state;
}

static void nfa_add_transition(NFA_State *from, const char *symbol, NFA_State *to) {
  NFA_State **to_list = shget(from->transition, symbol);
  arrput(to_list, to);
  shput(from->transition, symbol, to_list);
}

static void nfa_state_free(NFA_State *state, Traversal_Map **p_free_map) {
	for (int i = 0; i < hmlen(state->transition); ++i) {
		NFA_State **neighbors = state->transition[i].value;

		for (int j = 0; j < arrlen(neighbors); ++j) {
			NFA_State *neighbor = neighbors[j];

			if (hmget(*p_free_map, neighbor) == false) {
				nfa_state_free(neighbor, p_free_map);
			}
		}
	}

  for (int i = 0; i < hmlen(state->transition); ++i) {
    arrfree(state->transition[i].value);
  }
  hmfree(state->transition);

	hmput(*p_free_map, state, true);
	free(state);
}

static void nfa_state_print(NFA_State *state, Traversal_Map **p_map) {
	hmput(*p_map, state, true);

	for (int i = 0; i < hmlen(state->transition); ++i) {
		const char *symbol = state->transition[i].key;
		NFA_State **neighbors = state->transition[i].value;

		for (int j = 0; j < arrlen(neighbors); ++j) {
			NFA_State *neighbor = neighbors[j];

			printf("%p ---- %s ----> %p\n", (void *)state, symbol, (void *)neighbor);

			if (hmget(*p_map, neighbor) == false) {
				nfa_state_print(neighbor, p_map);
			}
		}
	}
}

void nfa_build_from_regex(NFA *nfa, const char *regex) {
  NFA_State *state0 = nfa_state_init();
  NFA_State *state1 = nfa_state_init();

  nfa_add_transition(state0, regex, state1);

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
	arrfree(empty_reachables);

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

	NFA_State **other_start_states = state_next(nfa->start_state, NFA_EPSILON);
	merge_state_list(&current_states, other_start_states);

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

void nfa_free(NFA *nfa, bool owned) {
	if (owned) {
		Traversal_Map *map = NULL;
		nfa_state_free(nfa->start_state, &map);
		hmfree(map);
	}

  hmfree(nfa->accepting_states);
}

void nfa_print(NFA *nfa) {
	Traversal_Map *map = NULL;

	puts("--------------------");

	printf("Start state: %p\n", (void *)nfa->start_state);
	puts("Accepting state(s):");
	for (int i = 0; i < hmlen(nfa->accepting_states); ++i) {
		printf("%p\n", (void *)nfa->accepting_states[i].key);
	}

	puts("--------------------");
	nfa_state_print(nfa->start_state, &map);
	puts("--------------------");

	hmfree(map);
}

static void copy_accepting_states(NFA *nfa1, NFA *nfa2) {
	for (int i = 0; i < hmlen(nfa2->accepting_states); ++i) {
		NFA_State *accepting_state = nfa2->accepting_states[i].key;
		hmput(nfa1->accepting_states, accepting_state, true);
	}
}

void nfa_concat(NFA *nfa1, NFA *nfa2) {
	NFA_State **to_be_removed = NULL;

	for (int i = 0; i < hmlen(nfa1->accepting_states); ++i) {
		NFA_State *accepting_state = nfa1->accepting_states[i].key;
		nfa_add_transition(accepting_state, NFA_EPSILON,
nfa2->start_state);

		arrput(to_be_removed, accepting_state);
	}

	for (int i = 0; i < arrlen(to_be_removed); ++i) {
		bool removed = hmdel(nfa1->accepting_states, to_be_removed[i]);
		assert(removed == true);
	}
	arrfree(to_be_removed);

	copy_accepting_states(nfa1, nfa2);
}

void nfa_union(NFA *nfa1, NFA *nfa2) {
	NFA_State *start_state = nfa_state_init();

	nfa_add_transition(start_state, NFA_EPSILON, nfa1->start_state);
	nfa_add_transition(start_state, NFA_EPSILON, nfa2->start_state);

	nfa1->start_state = start_state;

	copy_accepting_states(nfa1, nfa2);
}
