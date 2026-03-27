#include "Rexer.h"
#include <assert.h>
#include <stdio.h>

void rexer_set_rule(Rexer *rexer, const char *regex, int token) {
	arrput(rexer->rules, ((Rexer_Rule){
				.regex = strdup(regex),
				.nfa = nfa_from_regex(regex),
				.token = token
	}));
}

void rexer_set_error_handler(Rexer *rexer, Rexer_Error_Func handler, void *user_data) {
	rexer->error_handler = (Rexer_Error_Handler){
		.handler = handler,
		.user_data = user_data
	};
}

void rexer_free(Rexer *rexer) {
	for (int i = 0; i < arrlen(rexer->rules); ++i) {
		Rexer_Rule rule = rexer->rules[i];
		free((void *)rule.regex);
		nfa_free(rule.nfa);
	}

	arrfree(rexer->rules);

	if (rexer->line_starts != NULL) {
		arrfree(rexer->line_starts);
	}

	if (rexer->source_dup != NULL) {
		free(rexer->source_dup);
	}

}

static Rexer_Rule *accepter(Rexer *rexer, const char *string) {
	Rexer_Rule *result = NULL;

	for (int i = 0; i < arrlen(rexer->rules); ++i) {
		Rexer_Rule rule = rexer->rules[i];
		if (nfa_accepts(rule.nfa, string)) {
			arrput(result, rule);
		}
	}

	return result;
}

static size_t *get_line_starts(const char *string) {
	size_t *starts = NULL;
	size_t length = strlen(string);

	if (string[0] != '\n') {
		// First line starts at the first character
		arrput(starts, 0);
	} else {
		// First line never even existed
		arrput(starts, -1);
	}

	for (size_t i = 0; i < length - 1; ++i) {
		if (string[i] == '\n') {
			arrput(starts, i + 1);
		}
	}

	return starts;
}

static void get_line_column(size_t *line_starts, size_t index, size_t *line, size_t *column) {
	*line = 0;
	while (index > line_starts[*line] && *line != (size_t)arrlen(line_starts) - 1 && index >= line_starts[*line + 1]) ++(*line);

	*column = index - line_starts[*line];
}

// `start` is inclusive and `end` is exclusive
static char *string_duplicate(const char *string, size_t start, size_t end) {
	size_t length = end - start;
	char *result = malloc(sizeof(char) * (length + 1));

	for (size_t i = 0; i < length; ++i) {
		result[i] = string[start + i];
	}
	result[length] = '\0';

	return result;
}

bool rexer_has_next(Rexer *rexer) {
	if (rexer->source_length == 0) {
		rexer->source_length = strlen(rexer->source);
	}

	return rexer->start < rexer->source_length;
}

Rexer_Token rexer_next(Rexer *rexer) {

	Rexer_Token result = {0};

	if (rexer->source_length == 0) {
		rexer->source_length = strlen(rexer->source);
	}

	if (rexer->line_starts == NULL) {
		rexer->line_starts = get_line_starts(rexer->source);
	}

	if (rexer->source_dup == NULL) {
		rexer->source_dup = strdup(rexer->source);
	}

	size_t end = rexer->start + 1;
  Rexer_Rule *previous_accepters = NULL;

  while (end <= rexer->source_length) {
    char c = rexer->source_dup[end];
    rexer->source_dup[end] = '\0';

    Rexer_Rule *current_accepters = accepter(rexer, rexer->source_dup + rexer->start);
    rexer->source_dup[end] = c;

    if (arrlen(current_accepters) == 0)
      break;

    arrfree(previous_accepters);
    previous_accepters = current_accepters;

    ++end;
  }
  --end;

	result.start.index = rexer->start;
	result.end.index = end - 1;

  get_line_column(rexer->line_starts, rexer->start, &result.start.line,
                  &result.start.column);

  if (!previous_accepters) {
    if (rexer->error_handler.handler) {
      ++end;

      result.end.index = end - 1;
      get_line_column(rexer->line_starts, end - 1, &result.end.line,
                      &result.end.column);

      char *err_lexeme = string_duplicate(rexer->source, rexer->start, end);
			rexer->error_handler.handler(err_lexeme, result.start,
																	 rexer->error_handler.user_data);
			result.token = REXER_ERROR_TOKEN;

      free(err_lexeme);
    } else {
      fprintf(stderr, "Lexical Error at line %lu, column %lu\n",
              result.start.line, result.start.column);
      exit(1);
    }
  }

  else {
		Rexer_Rule rule = previous_accepters[0];
    get_line_column(rexer->line_starts, end - 1, &result.end.line,
                    &result.end.column);

		result.token = rule.token;
		result.lexeme = string_duplicate(rexer->source, rexer->start, end);
  }

  arrfree(previous_accepters);

  rexer->start = end;

	return result;
}
