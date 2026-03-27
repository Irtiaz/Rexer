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
  rexer_set_rule(&rexer, "[bc]+", TOK_B);
  rexer_set_error_handler(&rexer, error_handler, NULL);

  while (rexer_has_next(&rexer)) {
    Rexer_Token token_info = rexer_next(&rexer);

    switch (token_info.token) {

    case TOK_A: {
      puts("a handler");
      puts(token_info.lexeme);
      printf("index: %lu\n", token_info.start.index);
      printf("line: %lu, column: %lu\n", token_info.start.line, token_info.start.column);
      printf("line: %lu, column: %lu\n", token_info.end.line, token_info.end.column);
    } break;

    case TOK_B: {
      puts("b handler");
      puts(token_info.lexeme);
      printf("index: %lu\n", token_info.start.index);
      printf("line: %lu, column: %lu\n", token_info.start.line, token_info.start.column);
      printf("line: %lu, column: %lu\n", token_info.end.line, token_info.end.column);
    } break;

    }

		free(token_info.lexeme);
  }

  rexer_free(&rexer);
  return 0;
}
