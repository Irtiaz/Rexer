#include "NFA.h"
#include "stb_ds.h"
#include <assert.h>
#include <stdio.h>

typedef enum {
  TOK_STAR,
  TOK_PLUS,
	TOK_OPTIONAL,
  TOK_LPAREN,
  TOK_RPAREN,
	TOK_LSQUARE,
	TOK_RSQUARE,
	TOK_RANGE,
  TOK_ANY,
  TOK_UNION,
  TOK_CHAR,
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
  char value;        // Only valid for TOK_CHAR
  size_t pair_index; // Only valid for paired tokens like LPAREN-RPAREN

  size_t location;
} Regex_Token;

static NFA_State *nfa_state_init(void) {
  NFA_State *state = malloc(sizeof(NFA_State));
  state->transition = NULL;
  sh_new_arena(state->transition);

  return state;
}

static void nfa_free_private(NFA *nfa, bool owned);

static void nfa_add_transition(NFA_State *from, const char *symbol,
                               NFA_State *to) {
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
static void nfa_collect_all_states(NFA_State *state, Traversal_Map **p_map,
                                   NFA_State ***p_list) {
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
// That can be directly viewed with
// https://dreampuf.github.io/GraphvizOnline/?engine=dot
static void nfa_state_print(NFA_State *state, Traversal_Map **p_map) {
  hmput(*p_map, state, true);

  for (int i = 0; i < hmlen(state->transition); ++i) {
    const char *symbol = state->transition[i].key;
    NFA_State **neighbors = state->transition[i].value;

    for (int j = 0; j < arrlen(neighbors); ++j) {
      NFA_State *neighbor = neighbors[j];

      printf("\t\"%p\" -> \"%p\" [label=\"%s\"]\n", (void *)state,
             (void *)neighbor, symbol);

      if (hmget(*p_map, neighbor) == false) {
        nfa_state_print(neighbor, p_map);
      }
    }
  }
}

NFA *nfa_from_symbol(const char *symbol) {
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

    if (tokens[i].kind == TOK_LPAREN)
      ++global_counter;
    else
      --global_counter;

    if (global_counter < 0) {
      fprintf(stderr, "Stray ')' found\n%s\n", regex);
      fprintf(stderr, "%*s^~~~~~~\n", (int)tokens[i].location, " ");
      error = true;
    }

    if (tokens[i].kind != TOK_LPAREN)
      continue;

    bool pair_found = false;
    int counter = 1;

    for (int j = i + 1; j < arrlen(tokens); ++j) {
      if (tokens[j].kind != TOK_LPAREN && tokens[j].kind != TOK_RPAREN) {
        continue;
      }

      if (tokens[j].kind == TOK_LPAREN)
        ++counter;
      else
        --counter;

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

  if (error)
    exit(1);
}

static Regex_Token *tokenize(const char *regex) {
	Regex_Token *tokens = NULL;

  size_t i = 0;

	int last_lsquare_index = -1;

  while (regex[i] != '\0') {
    char c = regex[i];

		if (last_lsquare_index >= 0 && c != ']' && c != '-') {
			// We are in the [] block
      arrput(tokens,
             ((Regex_Token){.kind = TOK_CHAR, .location = i, .value = c}));
			++i;
			continue;
		}

		if (last_lsquare_index >= 0 && c == '-') {
      arrput(tokens, ((Regex_Token){.kind = TOK_RANGE, .location = i}));
			++i;
			continue;
		}

    if (c == '(') {
      arrput(tokens, ((Regex_Token){.kind = TOK_LPAREN, .location = i}));
    }

    else if (c == ')') {
      arrput(tokens, ((Regex_Token){.kind = TOK_RPAREN, .location = i}));
    }

    else if (c == '[') {
      arrput(tokens, ((Regex_Token){.kind = TOK_LSQUARE, .location = i}));

			last_lsquare_index = arrlen(tokens) - 1;
    }

    else if (c == ']') {
			if (last_lsquare_index < 0) {
        fprintf(stderr, "Stray ']' encountered\n%s\n", regex);
        fprintf(stderr, "%*s^~~~~~~\n", (int)i, " ");
        exit(1);
			}

      arrput(tokens, ((Regex_Token){.kind = TOK_RSQUARE, .location = i}));
			
			tokens[last_lsquare_index].pair_index = arrlen(tokens) - 1;
			arrlast(tokens).pair_index = last_lsquare_index;

			last_lsquare_index = -1;
    }


    else if (c == '+') {
      arrput(tokens, ((Regex_Token){.kind = TOK_PLUS, .location = i}));
    }

    else if (c == '*') {
      arrput(tokens, ((Regex_Token){.kind = TOK_STAR, .location = i}));
    }

    else if (c == '?') {
      arrput(tokens, ((Regex_Token){.kind = TOK_OPTIONAL, .location = i}));
    }

    else if (c == '.') {
      arrput(tokens, ((Regex_Token){.kind = TOK_ANY, .location = i}));
    }

    else if (c == '|') {
      arrput(tokens, ((Regex_Token){.kind = TOK_UNION, .location = i}));
    }

    else if (c != '\\') {
      arrput(tokens,
             ((Regex_Token){.kind = TOK_CHAR, .location = i, .value = c}));
    }

    else {
      if (regex[i + 1] == '\0') {
        fprintf(stderr, "Unfinished escape character encountered\n%s\n", regex);
        fprintf(stderr, "%*s^~~~~~~\n", (int)i, " ");
        exit(1);
      }

      else {
        arrput(tokens, ((Regex_Token){.kind = TOK_CHAR,
                                      .location = i + 1,
                                      .value = regex[i + 1]}));
      }

      ++i;
    }

    ++i;
  }

	if (last_lsquare_index >= 0) {
		Regex_Token last_lsquare = tokens[last_lsquare_index];
		fprintf(stderr, "Stray '[' encountered\n%s\n", regex);
		fprintf(stderr, "%*s^~~~~~~\n", (int)last_lsquare.location, " ");
		exit(1);
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

    case TOK_OPTIONAL: {
      puts("TOK_OPTIONAL");
    } break;

    case TOK_LPAREN: {
      printf("TOK_LPAREN, pair: %lu\n", token.pair_index);
    } break;

    case TOK_RPAREN: {
      printf("TOK_RPAREN, pair: %lu\n", token.pair_index);
    } break;

    case TOK_LSQUARE: {
      printf("TOK_LSQUARE, pair: %lu\n", token.pair_index);
    } break;

    case TOK_RSQUARE: {
      printf("TOK_RSQUARE, pair: %lu\n", token.pair_index);
    } break;

    case TOK_RANGE: {
      puts("TOK_RANGE");
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

static NFA *parse_union_expression(const char *regex, Regex_Token *tokens,
                                   size_t start, size_t end);

// Parses from the `tokens` array from start (inclusive) to end (exclusive)
// It might be a single character, or might be NFA_ANY
static NFA *parse_unary_expression(const char *regex, Regex_Token *tokens,
                                   size_t start) {

  Regex_Token start_token = tokens[start];
  Token_Kind start_kind = start_token.kind;

  if (start_kind != TOK_CHAR && start_kind != TOK_ANY) {
    fprintf(stderr, "Expected a character instead of %c\n%s\n", regex[start_token.location], regex);
    fprintf(stderr, "%*s^~~~~~~\n", (int)start, " ");
		exit(1);
  }

  char symbol[20] = {0};
  if (start_kind == TOK_CHAR) {
    symbol[0] = start_token.value;
  } else {
    strcpy(symbol, NFA_ANY);
  }

  NFA *nfa = nfa_from_symbol(symbol);

  return nfa;
}

static bool is_unary_token(Regex_Token token) {
	if (token.kind == TOK_STAR) return true;
	if (token.kind == TOK_PLUS) return true;
	if (token.kind == TOK_OPTIONAL) return true;
	return false;
}


// Parses from the `tokens` array from start (inclusive) to end (exclusive)
static NFA *parse_range_expression(const char *regex, Regex_Token *tokens,
																		size_t start, size_t end) {
	(void)regex;

	assert(end - start == 3);
	assert(tokens[start].kind == TOK_CHAR);
	assert(tokens[start + 1].kind == TOK_RANGE);
	assert(tokens[start + 2].kind == TOK_CHAR);

	char a = tokens[start].value;
	char b = tokens[start + 2].value;

	char min = a < b? a : b;
	char max = a > b? a : b;

	char symbol[2] = {0};
	symbol[0] = min;

	NFA *nfa = nfa_from_symbol(symbol);

	for (char c = min + 1; c <= max; ++c) {
		symbol[0] = c;

		NFA *current = nfa_from_symbol(symbol);
		nfa_union(nfa, current);
		nfa_free_private(current, false);
	}

	return nfa;
}


// Parses from the `tokens` array from start (inclusive) to end (exclusive)
static NFA *parse_set_expression(const char *regex, Regex_Token *tokens,
																		size_t start, size_t end) {

	NFA *nfa = NULL;

	size_t i = start;
	while (i < end) {
		NFA *current_nfa;

		if (i < end - 1 && tokens[i + 1].kind == TOK_RANGE) {
			if (i + 1 == end - 1) {
          fprintf(stderr, "Missing right operand for -\n%s\n", regex);
          fprintf(stderr, "%*s^~~~~~~\n", (int)tokens[end].location, " ");
          exit(1);
			}

			current_nfa = parse_range_expression(regex, tokens, i, i + 3);

			i += 3;
		}

		else {
			char symbol[2] = {0};
			symbol[0] = tokens[i].value;
			current_nfa = nfa_from_symbol(symbol);
			++i;
		}

		if (nfa == NULL) nfa = current_nfa;
		else {
			nfa_union(nfa, current_nfa);
			nfa_free_private(current_nfa, false);
		}

	}

	return nfa;
}

// Parses from the `tokens` array from start (inclusive) to end (exclusive)
static NFA *parse_concat_expression(const char *regex, Regex_Token *tokens,
																		size_t start, size_t end) {
	NFA *nfa = NULL;

  size_t i = start;
  while (i < end) {
    Regex_Token token = tokens[i];
    NFA *next;

    size_t next_i = i + 1;

    if (token.kind == TOK_LPAREN) {
      size_t pair = token.pair_index;
      next = parse_union_expression(regex, tokens, i + 1, pair);
      next_i = pair + 1;
    }

		else if (token.kind == TOK_LSQUARE) {
			size_t pair = token.pair_index;
			next = parse_set_expression(regex, tokens, i + 1, pair);
			next_i = pair + 1;
		}

    else {
      next = parse_unary_expression(regex, tokens, i);
    }

    while (next_i < end && is_unary_token(tokens[next_i])) {
      if (tokens[next_i].kind == TOK_STAR)
        nfa_kleene_star(next);
      else if (tokens[next_i].kind == TOK_PLUS)
        nfa_kleene_plus(next);
			else
				nfa_optional(next);
				
      ++next_i;
    }

    if (nfa == NULL)
      nfa = next;
    else {
      nfa_concat(nfa, next);
      nfa_free_private(next, false);
    }

    i = next_i;
  }

  return nfa;
}

// Parses from the `tokens` array from start (inclusive) to end (exclusive)
static NFA *parse_union_expression(const char *regex, Regex_Token *tokens,
                                   size_t start, size_t end) {

  NFA *nfa = NULL;

  size_t i = start;
  while (i < end) {
    Regex_Token token = tokens[i];
    if (token.kind == TOK_LPAREN) {
      i = token.pair_index;
    }

    else if (token.kind == TOK_UNION) {
      if (nfa == NULL) {
        if (i == start) {
          fprintf(stderr, "Missing left operand for |\n%s\n", regex);
          fprintf(stderr, "%*s^~~~~~~\n", (int)token.location, " ");
          exit(1);
        }

        nfa = parse_union_expression(regex, tokens, start, i);
      }

      if (i == end - 1) {
        fprintf(stderr, "Missing right operand for |\n%s\n", regex);
        fprintf(stderr, "%*s^~~~~~~\n", (int)token.location, " ");
        exit(1);
      }

      NFA *right = parse_union_expression(regex, tokens, i + 1, end);
      nfa_union(nfa, right);
      nfa_free_private(right, false);

      break;
    }

    ++i;
  }

  if (nfa == NULL) {
    nfa = parse_concat_expression(regex, tokens, start, end);
  }

  return nfa;
}

NFA *nfa_from_regex(const char *regex) {
  Regex_Token *tokens = tokenize(regex);
  NFA *nfa = parse_union_expression(regex, tokens, 0, arrlen(tokens));
  arrfree(tokens);

  return nfa;
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

static bool not_newline(const char *symbol) {
	if (symbol[0] == '\r') return false;
	if (symbol[0] == '\n') return false;
	return true;
}

static NFA_State **state_next_recursive(NFA_State *state, const char *symbol, Traversal_Map **p_map) {
	hmput(*p_map, state, true);

  NFA_State **nexts = duplicate_list(shget(state->transition, symbol));
  if (strcmp(symbol, NFA_EPSILON) && not_newline(symbol)) {
    merge_temp_list(&nexts, duplicate_list(shget(state->transition, NFA_ANY)));
  }

  NFA_State **empty_reachables = NULL;
  for (int i = 0; i < arrlen(nexts); ++i) {
		if (hmget(*p_map, nexts[i]) == false) {
				NFA_State **epsilons = state_next_recursive(nexts[i], NFA_EPSILON, p_map);
				merge_temp_list(&empty_reachables, epsilons);
		}
  }
  merge_temp_list(&nexts, empty_reachables);

  return nexts;
}

static NFA_State **state_next(NFA_State *state, const char *symbol) {
	Traversal_Map *map = NULL;
	NFA_State **nexts = state_next_recursive(state, symbol, &map);
	hmfree(map);

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

static void nfa_free_private(NFA *nfa, bool owned) {
  if (owned) {
    Traversal_Map *map = NULL;
    nfa_state_free(nfa->start_state, &map);
    hmfree(map);
  }

  hmfree(nfa->accepting_states);

  free(nfa);
}

void nfa_free(NFA *nfa) {
	nfa_free_private(nfa, true);
}

static NFA *nfa_deep_copy(NFA *nfa) {
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

  printf("\"%p\" [style=filled, fillcolor=lightblue]\n",
         (void *)nfa->start_state);
  for (int i = 0; i < hmlen(nfa->accepting_states); ++i) {
    printf("\"%p\" [style=filled, fillcolor=lightgreen]\n",
           (void *)nfa->accepting_states[i].key);
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
    nfa_add_transition(accepting_state, NFA_EPSILON, nfa2->start_state);

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

void nfa_optional(NFA *nfa) {
	NFA *empty = nfa_from_symbol(NFA_EPSILON);
	nfa_union(nfa, empty);
	nfa_free_private(empty, false);
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

  nfa_free_private(duplicate, false);

  nfa = duplicate;
}
