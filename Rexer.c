#include "Rexer.h"
#include <assert.h>
#include <stdio.h>

void rexer_register_rule(Rexer *rexer, const char *regex, Rexer_Handler handler, void *user_data) {
	arrput(rexer->rules, ((Rexer_Rule){
				.regex = strdup(regex),
				.handler = handler,
				.nfa = nfa_from_regex(regex),
				.user_data = user_data
	}));
}

void rexer_register_error_handler(Rexer *rexer, Rexer_Handler handler, void *user_data) {
	rexer->error_handler = (Rexer_Rule){
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

void rexer_start(Rexer *rexer, const char *source) {
	size_t length = strlen(source);
	char *string = strdup(source);
	size_t *line_starts = get_line_starts(source);

	size_t start = 0;
	while (start < length) {
		size_t end = start + 1;

		Rexer_Rule *previous_accepters = NULL;

		while (end <= length) {
			char c = string[end];
			string[end] = '\0';

			Rexer_Rule *current_accepters = accepter(rexer, string + start);
			string[end] = c;

			if (arrlen(current_accepters) == 0) break; 

			arrfree(previous_accepters);
			previous_accepters = current_accepters;

			++end;
		}
		--end;

		Rexer_Location start_location = {.index = start};
		Rexer_Location end_location = {.index = end - 1};

		get_line_column(line_starts, start, &start_location.line, &start_location.column);


		if (!previous_accepters) {
			if (rexer->error_handler.handler) {
				++end;

				end_location.index = end - 1;
				get_line_column(line_starts, end - 1, &end_location.line, &end_location.column);

				char *lexeme = string_duplicate(source, start, end);
				rexer->error_handler.handler(lexeme, start_location, end_location, rexer->error_handler.user_data);
				free(lexeme);
			}
			else {
				fprintf(stderr, "Lexical Error at line %lu, column %lu\n", start_location.line, start_location.column);
				exit(1);
			}
		}

		else {
			Rexer_Rule winner = previous_accepters[0];
			get_line_column(line_starts, end - 1, &end_location.line, &end_location.column);

			char *lexeme = string_duplicate(source, start, end);
			winner.handler(lexeme, start_location, end_location, winner.user_data);
			free(lexeme);
		}

		arrfree(previous_accepters);

		start = end;
	}

	arrfree(line_starts);
	free(string);
}
