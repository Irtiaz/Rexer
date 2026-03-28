#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include "Rexer.h"

typedef enum {
	PREPROCESSORS,
	NEWLINE,
	WHITESPACE,
	TYPEDEF,
	ENUM,
	CURL,
	PAREN,
	ID,
	COMMA,
	SEMICOLON,
	COLON,
	STAR,
	DOT,
	PLUS,
	INT,
	TYPE,
	STRING,
	ASSIGN,
	CHAR,
	AND,
	NOT,
	COMMENT,
	SWITCH,
	CASE,
	BREAK,
	RETURN
} Token;

void error_handler(const char *lexeme, Rexer_Location location,
                   void *user_data) {
  (void)user_data;
	printf(__FILE__":%lu:%lu:Unrecognized character %s\n", location.line + 1, location.column + 1, lexeme);
}

void report_token(const char *type, Rexer_Token token_info) {
	printf(__FILE__":%lu:%lu: %s %s\n", token_info.start.line + 1, token_info.start.column + 1, type, token_info.lexeme);
}

int main(void) {
	FILE *source_file = fopen(__FILE__, "r");
	char *source = NULL;
	char c;
	while ((c = fgetc(source_file)) != EOF) {
		arrput(source, c);
	}
	arrput(source, '\0');
	fclose(source_file);

  Rexer rexer = {.source = source};

  rexer_set_rule(&rexer, "//.*", COMMENT);
  rexer_set_rule(&rexer, "#.*", PREPROCESSORS);
  rexer_set_rule(&rexer, "\r|\n", NEWLINE);
  rexer_set_rule(&rexer, " +|\t+", WHITESPACE);
  rexer_set_rule(&rexer, "typedef", TYPEDEF);
  rexer_set_rule(&rexer, "enum", ENUM);
  rexer_set_rule(&rexer, "[{}]", CURL);
  rexer_set_rule(&rexer, "[()]", PAREN);
  rexer_set_rule(&rexer, ",", COMMA);
  rexer_set_rule(&rexer, ";", SEMICOLON);
  rexer_set_rule(&rexer, ":", COLON);
  rexer_set_rule(&rexer, "\\*", STAR);
	rexer_set_rule(&rexer, "void|int", TYPE);
	rexer_set_rule(&rexer, "switch", SWITCH);
	rexer_set_rule(&rexer, "case", CASE);
	rexer_set_rule(&rexer, "break", BREAK);
	rexer_set_rule(&rexer, "return", RETURN);
	rexer_set_rule(&rexer, "\x22.*\x22", STRING);
	rexer_set_rule(&rexer, "\\.", DOT);
	rexer_set_rule(&rexer, "\\+", PLUS);
	rexer_set_rule(&rexer, "=", ASSIGN);
	rexer_set_rule(&rexer, "'(.|\\\\0)'", CHAR);
	rexer_set_rule(&rexer, "[1-9][0-9]*|0", INT);
	rexer_set_rule(&rexer, "&", AND);
	rexer_set_rule(&rexer, "!", NOT);
	rexer_set_rule(&rexer, "[a-zA-Z_][a-zA-Z0-9_]*", ID);

  rexer_set_error_handler(&rexer, error_handler, NULL);

  while (rexer_has_next(&rexer)) {
    Rexer_Token token_info = rexer_next(&rexer);
		Token token = token_info.token;

    switch (token) {

		case COMMENT: {
			// Ignore
    } break;

		case PREPROCESSORS: {
			report_token("PREPROCESSORS", token_info);
    } break;

		case NEWLINE: {
			// Ignore
		} break;

		case WHITESPACE: {
			// Ignore
		} break;

		case TYPEDEF: {
			report_token("TYPEDEF", token_info);
		} break;

		case ENUM: {
			report_token("ENUM", token_info);
		} break;

		case CURL: {
			report_token("CURL", token_info);
		} break;

		case PAREN: {
			report_token("PAREN", token_info);
		} break;

		case COMMA: {
			report_token("COMMA", token_info);
		} break;

		case SEMICOLON: {
			report_token("SEMICOLON", token_info);
		} break;

		case COLON: {
			report_token("COLON", token_info);
		} break;

		case STAR: {
			report_token("STAR", token_info);
		} break;

		case STRING: {
			report_token("STRING", token_info);
		} break;

		case ASSIGN: {
			report_token("ASSIGN", token_info);
		} break;

		case CHAR: {
			report_token("CHAR", token_info);
		} break;

		case ID: {
			report_token("ID", token_info);
		} break;

		case DOT: {
			report_token("DOT", token_info);
		} break;

		case PLUS: {
			report_token("PLUS", token_info);
		} break;

		case INT: {
			report_token("INT", token_info);
		} break;

		case TYPE: {
			report_token("TYPE", token_info);
		} break;

		case SWITCH: {
			report_token("SWITCH", token_info);
		} break;

		case CASE: {
			report_token("CASE", token_info);
		} break;

		case BREAK: {
			report_token("BREAK", token_info);
		} break;

		case RETURN: {
			report_token("RETURN", token_info);
		} break;

		case AND: {
			report_token("AND", token_info);
		} break;

		case NOT: {
			report_token("NOT", token_info);
		} break;

    }

		free(token_info.lexeme);
  }

  rexer_free(&rexer);
	arrfree(source);
  return 0;
}
