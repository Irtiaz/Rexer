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

Rexer_Rule rexer_next(Rexer *rexer, const char **lexeme,
									Rexer_Location *start_location,
									Rexer_Location *end_location) {

	Rexer_Rule result = {0};

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

	start_location->index = rexer->start;
	end_location->index = end - 1;

  get_line_column(rexer->line_starts, rexer->start, &start_location->line,
                  &start_location->column);

  if (!previous_accepters) {
    if (rexer->error_handler.handler) {
      ++end;

      end_location->index = end - 1;
      get_line_column(rexer->line_starts, end - 1, &end_location->line,
                      &end_location->column);

      char *err_lexeme = string_duplicate(rexer->source, rexer->start, end);
      rexer->error_handler.handler(err_lexeme, *start_location,
                                   rexer->error_handler.user_data);
			result.token = REXER_ERROR_TOKEN;

      free(err_lexeme);

			*lexeme = NULL;
    } else {
      fprintf(stderr, "Lexical Error at line %lu, column %lu\n",
              start_location->line, start_location->column);
      exit(1);
    }
  }

  else {
    result = previous_accepters[0];
    get_line_column(rexer->line_starts, end - 1, &end_location->line,
                    &end_location->column);

    *lexeme = string_duplicate(rexer->source, rexer->start, end);
  }

  arrfree(previous_accepters);

  rexer->start = end;

	return result;
}

/* void rexer_start(Rexer *rexer, const char *source) { */

/* 	rexer->source = source; */
/* 	rexer->start = 0; */
/* 	rexer->source_length = strlen(rexer->source); */

/* 	while (rexer->start < rexer->source_length) { */
/* 		char *lexeme; */
/* 		Rexer_Location start_location, end_location; */

/* 		Rexer_Rule rule = rexer_next(rexer, (const char **)&lexeme, &start_location, &end_location); */

/* 		free(lexeme); */
/* 	} */
/* } */

