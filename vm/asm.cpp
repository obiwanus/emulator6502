// ================== Assembler ====================
#include <stdio.h>
#include <stdlib.h>

#define MAXLINE 1000

enum TokenType {
  Token_EndOfStream = 0,

  Token_Identifier,
  Token_Label,
  Token_Hash,
  Token_HexNumber,
  Token_DecNumber,

  Token_SyntaxError,
};

struct Token {
  TokenType type;
  int text_length;
  char *text;
};

struct Tokenizer {
  char *at;
  int line_num;
};

internal char *ReadFileIntoString(char *filename) {
  char *result = NULL;
  FILE *file = fopen(filename, "rb");
  if (file == NULL) {
    print("Couldn't open file %s\n", filename);
    exit(1);
  }

  fseek(file, 0, SEEK_END);
  int file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  result = (char *)malloc(file_size + 1);
  fread(result, file_size, 1, file);
  fclose(file);

  result[file_size] = '\0';

  return result;
}

inline bool IsEndOfLine(char c) { return (c == '\n' || c == '\r'); }

inline bool IsWhitespace(char c) {
  return (c == ' ' || c == '\t' || IsEndOfLine(c));
}

void EatWhiteSpace(Tokenizer *tokenizer) {
  for (;;) {
    char c = *tokenizer->at;
    if (IsWhitespace(c)) {
      if (c == '\n') {
        tokenizer->line_num++;
      }
      tokenizer->at++;
    } else if (c == '/' && tokenizer->at[1] == '/') {
      tokenizer->at += 2;
      while (!IsEndOfLine(*tokenizer->at)) {
        tokenizer->at++;
      }
    } else {
      break;
    }
  }
}

Token GetToken(Tokenizer *tokenizer) {
  EatWhiteSpace(tokenizer);

  Token token = {};

  return token;
}

internal int LoadProgram(char *filename, int MemoryAddress) {
  char *file_contents = ReadFileIntoString(filename);

  Tokenizer tokenizer = {};
  tokenizer.at = file_contents;

  bool parsing = true;
  while (parsing) {
    Token token = GetToken(&tokenizer);
    if (token.type == Token_EndOfStream) {
      parsing = false;
    } else if (token.type == Token_SyntaxError) {
      // TODO
      print("Syntax error");
      exit(1);
    } else {
    }
  }

  return 0;
}
