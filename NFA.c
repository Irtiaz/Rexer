#include "NFA.h"
#include <assert.h>
#include <stdio.h>
#include "stb_ds.h"

typedef enum {
	TOK_STAR,
	TOK_PLUS,
	TOK_LPAREN,
	TOK_RPAREN,
	TOK_ANY,
	TOK_UNION,
	TOK_CHAR
} Token_Kind;

typedef struct {
	NFA_State *key;
	bool value;
} Traversal_Map;

typedef struct {
	NFA_State *key;
	NFA_State *value;
} State_Map;

typedef struct {
	Token_Kind kind;
	char value; // Only valid for TOK_CHAR
	size_t pair_index; // Only valid for paired tokens like LPAREN-RPAREN

	size_t location;
} Regex_Token;

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
	hmput(*p_free_map, state, true);

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

	free(state);
}

// Collect all states into a list starting from `state`
static void nfa_collect_all_states(NFA_State *state, Traversal_Map **p_map, NFA_State ***p_list) {
	hmput(*p_map, state, true);
	arrput(*p_list, state);

	for (int i = 0; i < hmlen(state->transition); ++i) {
		NFA_State **neighbors = state->transition[i].value;

		for (int j = 0; j < arrlen(neighbors); ++j) {
			NFA_State *neighbor = neighbors[j];

			if (hmget(*p_map, neighbor) == false) {
				nfa_collect_all_states(neighbor, p_map, p_list);
			}
		}
	}
}

// Take a `list` of states and duplicate all of them and
// return a mapping from the old state to the new one
static State_Map *nfa_state_duplicates(NFA_State **list) {
	State_Map *map = NULL;
	for (int i = 0; i < arrlen(list); ++i) {
		NFA_State *duplicate = nfa_state_init();
		hmput(map, list[i], duplicate);
	}

	return map;
}

// Prints a dot format Graphviz output
// That can be directly viewed with https://dreampuf.github.io/GraphvizOnline/?engine=dot
static void nfa_state_print(NFA_State *state, Traversal_Map **p_map) {
	hmput(*p_map, state, true);

	for (int i = 0; i < hmlen(state->transition); ++i) {
		const char *symbol = state->transition[i].key;
		NFA_State **neighbors = state->transition[i].value;

		for (int j = 0; j < arrlen(neighbors); ++j) {
			NFA_State *neighbor = neighbors[j];

			printf("\t\"%p\" -> \"%p\" [label=\"%s\"]\n", (void *)state, (void *)neighbor, symbol);

			if (hmget(*p_map, neighbor) == false) {
				nfa_state_print(neighbor, p_map);
			}
		}
	}
}

NFA *nfa_build_from_symbol(const char *symbol) {
	NFA *nfa = calloc(1, sizeof(NFA));
  NFA_State *state0 = nfa_state_init();
  NFA_State *state1 = nfa_state_init();

  nfa_add_transition(state0, symbol, state1);

  nfa->start_state = state0;
  hmput(nfa->accepting_states, state1, true);

	return nfa;
}

static void match_parens(const char *regex, Regex_Token *tokens) {
	bool error = false;

	int global_counter = 0;
	for (int i = 0; i < arrlen(tokens); ++i) {
		if (tokens[i].kind != TOK_LPAREN && tokens[i].kind != TOK_RPAREN) {
			continue;
		}

		if (tokens[i].kind == TOK_LPAREN) ++global_counter;
		else --global_counter;

		if (global_counter < 0) {
			fprintf(stderr, "Stray ')' found\n%s\n", regex);
			fprintf(stderr, "%*s^~~~~~~\n", (int)tokens[i].location, " ");
			error = true;
		}

		if (tokens[i].kind != TOK_LPAREN) continue;

		bool pair_found = false;
		int counter = 1;

		for (int j = i + 1; j < arrlen(tokens); ++j) {
			if (tokens[j].kind != TOK_LPAREN && tokens[j].kind != TOK_RPAREN) {
				continue;
			}

			if (tokens[j].kind == TOK_LPAREN) ++counter;
			else --counter;

			if (counter == 0) {
				tokens[i].pair_index = j;
				tokens[j].pair_index = i;

				pair_found = true;
				break;
			}
		}

		if (!pair_found) {
			fprintf(stderr, "Stray '(' found\n%s\n", regex);
			fprintf(stderr, "%*s^~~~~~~\n", (int)tokens[i].location, " ");
			error = true;
		}
	}

	if (error) exit(1);
}

static Regex_Token *tokenize(const char *regex) {
	Regex_Token *tokens = NULL;

	size_t i = 0;

	while (regex[i] != '\0') {
		char c = regex[i];

		if (c == '(') {
			arrput(tokens, ((Regex_Token){
				.kind = TOK_LPAREN,
				.location = i
			}));
		}

		else if (c == ')') {
			arrput(tokens, ((Regex_Token){
				.kind = TOK_RPAREN,
				.location = i
			}));
		}

		else if (c == '+') {
			arrput(tokens, ((Regex_Token){
				.kind = TOK_PLUS,
				.location = i
			}));
		}

		else if (c == '*') {
			arrput(tokens, ((Regex_Token){
				.kind = TOK_STAR,
				.location = i
			}));
		}

		else if (c == '.') {
			arrput(tokens, ((Regex_Token){
				.kind = TOK_ANY,
				.location = i
			}));
		}

		else if (c == '|') {
			arrput(tokens, ((Regex_Token){
				.kind = TOK_UNION,
				.location = i
			}));
		}

		else if (c != '\\') {
			arrput(tokens, ((Regex_Token){
				.kind = TOK_CHAR,
				.location = i,
				.value = c
			}));
		}

		else {
			if (regex[i + 1] == '\0') {
				fprintf(stderr, "Unfinished escape character encountered\n%s\n", regex);
				fprintf(stderr, "%*s^~~~~~~\n", (int)i, " ");
				exit(1);
			}

			else {
				arrput(tokens, ((Regex_Token){
						.kind = TOK_CHAR,
						.location = i + 1,
						.value = regex[i + 1]
				}));
			}

			++i;
		}

		++i;
	}

	match_parens(regex, tokens);
	
	return tokens;
}

void print_tokens(const char *regex) {
	Regex_Token *tokens = tokenize(regex);

	for (int i = 0; i < arrlen(tokens); ++i) {
		Regex_Token token = tokens[i];
		switch (token.kind) {
				case TOK_STAR: {
					puts("TOK_STAR");
				} break;

				case TOK_PLUS: {
					puts("TOK_PLUS");
				} break;

				case TOK_LPAREN: {
					printf("TOK_LPAREN, pair: %lu\n", token.pair_index);
				} break;

				case TOK_RPAREN: {
					printf("TOK_RPAREN, pair: %lu\n", token.pair_index);
				} break;

				case TOK_ANY: {
					puts("TOK_ANY");
				} break;

				case TOK_UNION: {
					puts("TOK_UNION");
				} break;

				case TOK_CHAR: {
					printf("TOK_CHAR: %c\n", token.value);
				} break;

		}
	}
	
	arrfree(tokens);
}

// Returns the next symbol character
// '\\' in case of any escape character
// Also sets `len` based on how much regex-characters were consumed
// Does NOT change regex
static char next_symbol_char(const char *regex, size_t *len) {
	if (*regex != '\\') {
		*len = 1;
		return *regex;
	}

	else {
		*len = 2;
		return '\\'; // A skip character, not important exactly what
	}
}

// Writes appropriate symbol string into symbol
// `size` is the size of the buffer for symbol (caller will set)
// Changes the regex parameter (basically chops away the next symbol)
static void chop_next_symbol(char *symbol, size_t size, const char **regex) {
	memset(symbol, 0, size);

	if (**regex != '\\') {
		if (**regex != '.') {
				symbol[0] = **regex;
		}
		else {
			strcpy(symbol, NFA_ANY);
		}

		(*regex)++;
	}

	else {
		symbol[0] = *(*regex + 1);
		(*regex) += 2;
	}
}

static NFA *nfa_build_concat_regex(const char *regex, size_t length) {
	char symbol[20] = {0};
	chop_next_symbol(symbol, sizeof(symbol), &regex);

	NFA *nfa = nfa_build_from_symbol(symbol);

	for (size_t i = 1; i < length && *regex; ++i) {
		chop_next_symbol(symbol, sizeof(symbol), &regex);
		NFA *nfa_next = nfa_build_from_symbol(symbol);
		nfa_concat(nfa, nfa_next);
		nfa_free(nfa_next, false);
	}

	return nfa;
}

static NFA *nfa_build_union_regex(const char *regex, size_t length) {
	NFA *nfa = NULL;
	size_t start = 0;

	size_t i = 0;
	while (i < length) {
		size_t next_len;
		char next = next_symbol_char(regex + i, &next_len);

		if (next == '|' || i == length - 1) {
			if (i == length - 1) {
				++i;
			}
			if (i == start) {
				fprintf(stderr, "Missing left operand for |\n");
				exit(1);
			}

			if (nfa == NULL) {
				nfa = nfa_build_concat_regex(regex + start, i - start);
			}
			else {
				NFA *nfa_next = nfa_build_concat_regex(regex + start, i - start);
				nfa_union(nfa, nfa_next);
				nfa_free(nfa_next, false);
			}

			start = i + 1;
		}

		i += next_len;
	}

	if (nfa == NULL) {
		nfa = nfa_build_concat_regex(regex, length);
	}

	return nfa;
}

NFA *nfa_build_from_regex(const char *regex) {
	return nfa_build_union_regex(regex, strlen(regex));
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

static void merge_temp_list(NFA_State ***p_list, NFA_State **temp) {
	merge_state_list(p_list, temp);
	arrfree(temp);
}


static NFA_State **duplicate_list(NFA_State **list) {
	NFA_State **result = NULL;
	for (int i = 0; i < arrlen(list); ++i) {
		arrput(result, list[i]);
	}
	return result;
}

static NFA_State **state_next(NFA_State *state, const char *symbol) {
  NFA_State **nexts = duplicate_list(shget(state->transition, symbol));
	if (strcmp(symbol, NFA_EPSILON)) {
		merge_temp_list(&nexts, duplicate_list(shget(state->transition, NFA_ANY)));
	}

  NFA_State **empty_reachables = NULL;
  for (int i = 0; i < arrlen(nexts); ++i) {
    NFA_State **epsilons = state_next(nexts[i], NFA_EPSILON);
    merge_temp_list(&empty_reachables, epsilons);
  }
  merge_temp_list(&nexts, empty_reachables);

  return nexts;
}

static NFA_State **state_list_next(NFA_State **state_list, const char *symbol) {
  NFA_State **result = NULL;

  for (int i = 0; i < arrlen(state_list); ++i) {
    NFA_State **nexts = state_next(state_list[i], symbol);
    merge_temp_list(&result, nexts);
  }

  return result;
}

bool nfa_accepts(NFA *nfa, const char *string) {
  NFA_State **current_states = NULL;
  arrput(current_states, nfa->start_state);

	NFA_State **other_start_states = state_next(nfa->start_state, NFA_EPSILON);
	merge_temp_list(&current_states, other_start_states);

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

	free(nfa);
}

NFA *nfa_deep_copy(NFA *nfa) {
	NFA_State **states = NULL;

	Traversal_Map *traversal_map = NULL;
	nfa_collect_all_states(nfa->start_state, &traversal_map, &states);
	hmfree(traversal_map);

	State_Map *state_map = nfa_state_duplicates(states);

	for (int i = 0; i < arrlen(states); ++i) {
		NFA_State *old_state = states[i];
		NFA_State *new_state = hmget(state_map, old_state);

		for (int j = 0; j < hmlen(old_state->transition); ++j) {
			const char *symbol = old_state->transition[j].key;
			NFA_State **neighbors = old_state->transition[j].value;

			for (int k = 0; k < arrlen(neighbors); ++k) {
				NFA_State *old_neighbor = neighbors[k];
				NFA_State *new_neighbor = hmget(state_map, old_neighbor);

				nfa_add_transition(new_state, symbol, new_neighbor);
			}
		}
	}
	arrfree(states);

	NFA *duplicate = calloc(1, sizeof(NFA));
	duplicate->start_state = hmget(state_map, nfa->start_state);

	for (int i = 0; i < hmlen(nfa->accepting_states); ++i) {
		NFA_State *accepting_state = nfa->accepting_states[i].key;
		NFA_State *dup_accept = hmget(state_map, accepting_state);
		hmput(duplicate->accepting_states, dup_accept, true);
	}

	hmfree(state_map);
	
	return duplicate;
}

void nfa_print(NFA *nfa) {
	Traversal_Map *map = NULL;

	puts("\n--------------------");
	puts("Digraph G {");

	printf("\"%p\" [style=filled, fillcolor=lightblue]\n", (void *)nfa->start_state);
	for (int i = 0; i < hmlen(nfa->accepting_states); ++i) {
		printf("\"%p\" [style=filled, fillcolor=lightgreen]\n", (void *)nfa->accepting_states[i].key);
	}

	nfa_state_print(nfa->start_state, &map);
	puts("}");
	puts("--------------------\n");

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

void nfa_kleene_star(NFA *nfa) {
	// Following the Thompson Construction for kleene star
	// https://en.wikipedia.org/wiki/Thompson%27s_construction

	NFA_State *new_start_state = nfa_state_init();
	NFA_State *new_accepting_state = nfa_state_init();

	nfa_add_transition(new_start_state, NFA_EPSILON, nfa->start_state);
	nfa_add_transition(new_start_state, NFA_EPSILON, new_accepting_state);

	NFA_State **to_be_removed = NULL;

	for (int i = 0; i < hmlen(nfa->accepting_states); ++i) {
		NFA_State *accepting_state = nfa->accepting_states[i].key;

		nfa_add_transition(accepting_state, NFA_EPSILON, nfa->start_state);
		nfa_add_transition(accepting_state, NFA_EPSILON, new_accepting_state);
		
		arrput(to_be_removed, accepting_state);
	}

	for (int i = 0; i < arrlen(to_be_removed); ++i) {
		(void)hmdel(nfa->accepting_states, to_be_removed[i]);
	}
	arrfree(to_be_removed);

	nfa->start_state = new_start_state;
	hmput(nfa->accepting_states, new_accepting_state, true);
}

void nfa_kleene_plus(NFA *nfa) {
	NFA *duplicate = nfa_deep_copy(nfa);

	nfa_kleene_star(duplicate);
	nfa_concat(nfa, duplicate);

	nfa_free(duplicate, false);

	nfa = duplicate;
}
