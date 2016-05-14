// ================== Assembler ====================
#include <stdio.h>
#include <stdlib.h>

#define MAXLINE 1000

enum TokenType {
  Token_Identifier = 0,
  Token_Label,
  Token_Hash,
  Token_HexNumber,
  Token_DecNumber,

  Token_EndOfStream,
  Token_SyntaxError,
};

struct Token {
  TokenType type;
  int text_length;
  char *text;
};

internal char *ReadFileIntoString(char *Filename) {
  char *result = NULL;
  FILE *file = fopen(Filename, "rb");
  if (file == NULL) {
    print("Couldn't open file %s\n", Filename);
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

internal int LoadProgram(char *Filename, int MemoryAddress) {

  char *file_contents = ReadFileIntoString(Filename);

  bool Parsing = true;

  // while (Parsing) {
  //   Token token = GetToken(Tokenizer);
  //   if (token.type == Token_EndOfStream) {
  //     Parsing = false;
  //   } else if (token.type == Token_SyntaxError) {
  //     exit(1);
  //   }
  // }

  return 0;
}
