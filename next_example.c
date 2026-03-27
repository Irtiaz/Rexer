#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include "Rexer.h"

enum { TOK_A, TOK_B };

void error_handler(const char *lexeme, Rexer_Location location,
                   void *user_data) {
  (void)user_data;
  puts("Error handler");
  puts(lexeme);
  printf("index: %lu\n", location.index);
  printf("line: %lu, column: %lu\n", location.line, location.column);
}

int main(void) {
  const char *source = "aaa\nccbb";
  Rexer rexer = {.source = source};

  rexer_set_rule(&rexer, "a+", TOK_A);
  rexer_set_rule(&rexer, "b*", TOK_B);
  rexer_set_error_handler(&rexer, error_handler, NULL);

  while (rexer_has_next(&rexer)) {
    const char *lexeme = NULL;
    Rexer_Location start, end;
    Rexer_Rule rule = rexer_next(&rexer, &lexeme, &start, &end);

    switch (rule.token) {

    case TOK_A: {
      puts("a handler");
      puts(lexeme);
      printf("index: %lu\n", start.index);
      printf("line: %lu, column: %lu\n", start.line, start.column);
      printf("line: %lu, column: %lu\n", end.line, end.column);
    } break;

    case TOK_B: {
      puts("b handler");
      puts(lexeme);
      printf("index: %lu\n", start.index);
      printf("line: %lu, column: %lu\n", start.line, start.column);
      printf("line: %lu, column: %lu\n", end.line, end.column);
    } break;

    }

		if (lexeme) free((void *)lexeme);
  }

  rexer_free(&rexer);
  return 0;
}
