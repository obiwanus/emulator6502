// ================== Assembler ====================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 1000

enum TokenType {
  Token_EndOfStream = 0,

  Token_Identifier,
  Token_Label,
  Token_Hash,
  Token_HexNumber,
  Token_DecNumber,
  Token_OpenParen,
  Token_CloseParen,
  Token_Comma,

  Token_SyntaxError,
};

struct Token {
  TokenType type;
  int length;
  char *text;
  int value;
};

struct Tokenizer {
  char *at;
  int line_num;
  char error_msg[500];
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

inline bool IsAlpha(char c) {
  return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

inline bool IsDecimal(char c) {
  return '0' <= c && c <= '9';
}

inline bool IsHex(char c) {
  return IsDecimal(c) || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
}

void EatWhiteSpace(Tokenizer *tokenizer) {
  for (;;) {
    char c = tokenizer->at[0];
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
  token.text = tokenizer->at;

  char c = *tokenizer->at;
  if (IsAlpha(c)) {
    while (!IsWhitespace(c) && c != ':' && c != '\0') {
      c = *++tokenizer->at;
      token.length++;
    }
    if (c == ':') {
      token.type = Token_Label;
      token.length--;  // don't include the colon
      tokenizer->at++;
    } else {
      token.type = Token_Identifier;
    }
  }
  else if (c == '#') {
    token.type = Token_Hash;
    tokenizer->at++;
  }
  else if (c == '(') {
    token.type = Token_OpenParen;
    tokenizer->at++;
  }
  else if (c == ')') {
    token.type = Token_CloseParen;
    tokenizer->at++;
  }
  else if (c == ',') {
    token.type = Token_Comma;
    tokenizer->at++;
  } else if (IsDecimal(c)) {
    token.type = Token_DecNumber;
    while (IsDecimal(c)) {
      c = *tokenizer->at++;
      token.length++;
    }
  } else if (c == '$') {
    token.type = Token_HexNumber;
    c = *++tokenizer->at;
    while (IsHex(c)) {
      c = *tokenizer->at++;
      token.length++;
    }
    if (token.length == 0) {
      token.type = Token_SyntaxError;
      sprintf(tokenizer->error_msg, "Number expected after $");
    }
  } else if (c == '\0') {
    token.type = Token_EndOfStream;
  } else {
    token.type = Token_SyntaxError;
    while (!IsWhitespace(c)) {
      c = *++tokenizer->at;
      token.length++;
    }
    sprintf(tokenizer->error_msg, "Unknown token '%.*s'", token.length,
            token.text);
  }

  return token;
}

internal int LoadProgram(char *filename, int MemoryAddress) {
  char *file_contents = ReadFileIntoString(filename);
  print("Assembling %s ...\n", filename);

  Tokenizer tokenizer = {};
  tokenizer.at = file_contents;
  tokenizer.line_num = 1;

  bool parsing = true;
  while (parsing) {
    Token token = GetToken(&tokenizer);
    if (token.type == Token_EndOfStream) {
      parsing = false;
    } else if (token.type == Token_SyntaxError) {
      print("Syntax error at line %d: %s\n", tokenizer.line_num,
            tokenizer.error_msg);
      exit(1);
    } else {
      // TODO: compile
    }
  }

  return 0;
}
