#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include "Rexer.h"

void a_handler(const char *lexeme, Rexer_Location start, Rexer_Location end, void *user_data) {
	(void)user_data;
	puts("a handler");
	puts(lexeme);
	printf("index: %lu\n", start.index);
	printf("line: %lu, column: %lu\n", start.line, start.column);
	printf("line: %lu, column: %lu\n", end.line, end.column);
}

void b_handler(const char *lexeme, Rexer_Location start, Rexer_Location end, void *user_data) {
	(void)user_data;
	puts(lexeme);
	puts("b handler");
	printf("index: %lu\n", start.index);
	printf("line: %lu, column: %lu\n", start.line, start.column);
	printf("line: %lu, column: %lu\n", end.line, end.column);
}

void error_handler(const char *lexeme, Rexer_Location location, void *user_data) {
	(void)user_data;
	puts("Error handler");
	puts(lexeme);
	printf("index: %lu\n", location.index);
	printf("line: %lu, column: %lu\n", location.line, location.column);
}

int main(void) {
	Rexer rexer = {0};

	const char *source = "aaa\nccbb";

	rexer_set_rule_handler(&rexer, "a+", a_handler, NULL);
	rexer_set_rule_handler(&rexer, "b*", b_handler, NULL);
	rexer_set_error_handler(&rexer, error_handler, NULL);

	rexer_start(&rexer, source);

	rexer_free(&rexer);
	return 0;
}
