// ================== Assembler ====================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

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
  int line_num;

  bool Equals(char *);
  void SyntaxError(char *);
};

struct Tokenizer {
  char *at;
  int line_num;
  char error_msg[500];

  Token *tokens;
  int num_tokens;
  int tokens_allocated;

  Token *NewToken();
};

// Addressing modes
#define AM_IMPLIED 0x00
#define AM_IMMEDIATE 0x01
#define AM_ABSOLUTE 0x02
#define AM_RELATIVE 0x04
#define AM_INDEXED_X 0x08
#define AM_INDEXED_Y 0x10
#define AM_INDIRECT_INDEXED_Y 0x20
#define AM_INDEXED_X_INDIRECT 0x40
#define AM_ALL 0xFF
#define AM_INDIRECT 0x100  // not included in AM_ALL

enum InstructionType {
  I_NOP = 0,
  I_ADC,
  I_AND,
  I_ASL,
  I_BCC,
  I_BCS,
  I_BEQ,
  I_BIT,
  I_BMI,
  I_BNE,
  I_BPL,
  I_BRK,
  I_BVC,
  I_CLC,
  I_CLD,
  I_CLI,
  I_CLV,
  // ...

  I_STA,
  I_LDA,
  I_JMP,
};

struct Instruction {
  InstructionType type;
  uint mode;
  int operand;
  Token *deferred_operand;  // to be looked up in the symbol table
};

struct SymbolTableEntry {
  char *symbol;
  int value;
  Instruction *instruction;
};

struct SymbolTable {
  char *symbol_space;
  char *free_space;
  int space_allocated;

  SymbolTableEntry *entries;
  int num_entries;
  int entries_allocated;

  SymbolTable();
  SymbolTableEntry *AddEntry(Token *, int);
  SymbolTableEntry *GetEntry(Token *);
  void AllocateSpaceIfNeeded(Token *);
};

struct Assembler {
  u8 *at_memory;
  Token *at_token;

  Token *NextToken();
  Token *PrevToken();
  Token *PeekToken();
  Token *RequireToken(TokenType);
  int RequireNumber();

  Instruction *instructions;
  int num_instructions;
  int instructions_allocated;

  Instruction *NewInstruction();

  void SyntaxError(char *);
};

struct CodeGenerator {
  u8 *at_memory;
  Instruction *at_instruction;
};

Token *Tokenizer::NewToken() {
  if (this->tokens_allocated == 0) {
    this->tokens_allocated = 1000;
    this->tokens = (Token *)malloc(this->tokens_allocated * sizeof(Token));
  } else if (this->num_tokens >= this->tokens_allocated - 2) {
    this->tokens_allocated *= 2;
    this->tokens =
        (Token *)realloc(this->tokens, this->tokens_allocated * sizeof(Token));
  }

  Token *result = this->tokens + this->num_tokens;
  this->num_tokens++;

  return result;
}

SymbolTable::SymbolTable() {
  this->space_allocated = 10000;
  this->symbol_space = (char *)malloc(this->space_allocated * sizeof(char));
  this->free_space = this->symbol_space;

  this->entries_allocated = 1000;
  this->num_entries = 0;
  this->entries = (SymbolTableEntry *)malloc(this->entries_allocated *
                                             sizeof(SymbolTableEntry));
}

void SymbolTable::AllocateSpaceIfNeeded(Token *token) {
  if (this->free_space - this->symbol_space >=
      this->space_allocated - token->length) {
    this->space_allocated *= 2;
    this->symbol_space = (char *)realloc(this->symbol_space,
                                         this->space_allocated * sizeof(char));
  }
  if (this->num_entries >= this->entries_allocated - 2) {
    this->entries_allocated *= 2;
    this->entries = (SymbolTableEntry *)realloc(
        this->entries, this->entries_allocated * sizeof(SymbolTableEntry));
  }
}

SymbolTableEntry *SymbolTable::AddEntry(Token *token, int value) {
  this->AllocateSpaceIfNeeded(token);

  // Find if there is an existing one
  // Yes, linear search, don't look at me like this
  for (int i = 0; i < this->num_entries; i++) {
    if (token->Equals(this->entries[i].symbol)) {
      token->SyntaxError("Identifier already declared");
    }
  }

  SymbolTableEntry *entry = this->entries + this->num_entries;
  this->num_entries++;

  entry->symbol = this->free_space;
  strncpy(entry->symbol, token->text, token->length);
  entry->symbol[token->length] = '\0';
  entry->value = value;
  entry->instruction = NULL;

  this->free_space += token->length + 1;

  return entry;
}

SymbolTableEntry *SymbolTable::GetEntry(Token *token) {
  SymbolTableEntry *entry = NULL;

  for (int i = 0; i < this->num_entries; i++) {
    if (token->Equals(this->entries[i].symbol)) {
      entry = this->entries + i;
      break;
    }
  }

  return entry;
}

bool Token::Equals(char *string) {
  for (int i = 0; i < this->length; i++) {
    if (string[i] == '\0' || tolower(string[i]) != tolower(this->text[i])) {
      return false;
    }
  }
  return true;
}

void Token::SyntaxError(char *string) {
  print("Syntax error at line %d: %s: '%.*s'\n", this->line_num, string,
        this->length, this->text);
  exit(1);
}

Token *Assembler::NextToken() {
  // keep at_token pointing at the current one, not the next
  this->at_token++;
  return this->at_token;
}

Token *Assembler::PrevToken() {
  this->at_token--;
  return this->at_token;
}

Token *Assembler::RequireToken(TokenType type) {
  Token *token = this->NextToken();
  if (token->type != type) {
    char *expected;
    if (type == Token_Identifier) {
      expected = "identifier";
    } else if (type == Token_Label) {
      expected = "label";
    } else if (type == Token_Hash) {
      expected = "'#'";
    } else if (type == Token_OpenParen) {
      expected = "'('";
    } else if (type == Token_CloseParen) {
      expected = "')'";
    } else if (type == Token_Comma) {
      expected = "comma";
    } else if (type == Token_DecNumber || type == Token_HexNumber) {
      expected = "number";
    } else {
      expected = "unknown token";  // hopefully we'll never reach this
    }
    char error[100];
    sprintf(error, "%s expected, got", expected);
    this->SyntaxError(error);
  }
  return token;
}

int atoi(char *string, int length, int base) {
  int value = 0;
  int digit = 0;
  int power = 1;
  for (int i = 0; i < length; i++) {
    int c = tolower(string[length - i - 1]);
    if ('a' <= c && c <= 'f') {
      digit = 10 + (c - 'a');
    } else {
      digit = c - '0';
    }
    value += digit * power;
    power *= base;
  }
  return value;
}

int Assembler::RequireNumber() {
  int value = 0;
  Token *token = this->NextToken();

  if (token->type != Token_HexNumber && token->type != Token_DecNumber) {
    this->SyntaxError("number expected, got");
  }

  if (token->type == Token_HexNumber) {
    value = atoi(token->text, token->length, 16);
  } else if (token->type == Token_DecNumber) {
    value = atoi(token->text, token->length, 10);
  }

  if (value > 0xFFFF) {
    this->SyntaxError("Number is too big");
  }

  return value;
}

Token *Assembler::PeekToken() { return (this->at_token + 1); }

Instruction *Assembler::NewInstruction() {
  if (this->instructions_allocated == 0) {
    this->instructions_allocated = 1000;
    this->instructions = (Instruction *)malloc(this->instructions_allocated *
                                               sizeof(Instruction));
  } else if (this->num_instructions >= this->instructions_allocated - 2) {
    this->instructions_allocated *= 2;
    this->instructions = (Instruction *)realloc(
        this->instructions, this->instructions_allocated * sizeof(Instruction));
  }

  Instruction *result = this->instructions + this->num_instructions;
  *result = {};
  this->num_instructions++;

  return result;
}

void Assembler::SyntaxError(char *string) {
  this->at_token->SyntaxError(string);
}

static char *ReadFileIntoString(char *filename) {
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

inline bool IsDecimal(char c) { return '0' <= c && c <= '9'; }

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

Token *GetToken(Tokenizer *tokenizer) {
  EatWhiteSpace(tokenizer);

  Token *token = tokenizer->NewToken();
  token->text = tokenizer->at;
  token->length = 0;
  token->line_num = tokenizer->line_num;  // for error reporting

  char c = *tokenizer->at;
  if (IsAlpha(c)) {
    while (!IsWhitespace(c) && c != ':' && c != '\0') {
      c = *++tokenizer->at;
      token->length++;
    }
    if (c == ':') {
      token->type = Token_Label;
      tokenizer->at++;
    } else {
      token->type = Token_Identifier;
    }
  } else if (c == '#') {
    token->type = Token_Hash;
    token->length = 1;
    tokenizer->at++;
  } else if (c == '(') {
    token->type = Token_OpenParen;
    token->length = 1;
    tokenizer->at++;
  } else if (c == ')') {
    token->type = Token_CloseParen;
    token->length = 1;
    tokenizer->at++;
  } else if (c == ',') {
    token->type = Token_Comma;
    token->length = 1;
    tokenizer->at++;
  } else if (IsDecimal(c)) {
    token->type = Token_DecNumber;
    while (IsDecimal(*tokenizer->at)) {
      tokenizer->at++;
      token->length++;
    }
  } else if (c == '$') {
    token->type = Token_HexNumber;
    tokenizer->at++;
    token->text++;  // skip the $
    while (IsHex(*tokenizer->at)) {
      tokenizer->at++;
      token->length++;
    }
    if (token->length == 0) {
      token->type = Token_SyntaxError;
      sprintf(tokenizer->error_msg, "Number expected after $");
    }
  } else if (c == '\0') {
    token->type = Token_EndOfStream;
  } else {
    token->type = Token_SyntaxError;
    while (!IsWhitespace(c)) {
      c = *++tokenizer->at;
      token->length++;
    }
    sprintf(tokenizer->error_msg, "Unknown token '%.*s'", token->length,
            token->text);
  }

  return token;
}

static int LoadProgram(char *filename, int memory_address) {
  char *file_contents = ReadFileIntoString(filename);
  print("Assembling %s ...\n", filename);

  Tokenizer tokenizer = {};
  tokenizer.at = file_contents;
  tokenizer.line_num = 1;

  bool tokenizing = true;
  while (tokenizing) {
    Token *token = GetToken(&tokenizer);
    if (token->type == Token_EndOfStream) {
      tokenizing = false;
    } else if (token->type == Token_SyntaxError) {
      print("Syntax error at line %d: %s\n", token->line_num,
            tokenizer.error_msg);
      exit(1);
    }
  }

  Assembler assembler = {};
  assembler.at_token = tokenizer.tokens;

  Token *token = assembler.at_token;
  SymbolTable symbol_table = SymbolTable();
  Instruction *instruction = assembler.instructions;

  bool parsing = true;

  while (parsing) {
    switch (token->type) {
      case Token_EndOfStream: {
        parsing = false;
      } break;

      case Token_Label: {
        SymbolTableEntry *entry = symbol_table.AddEntry(token, 0);
        entry->instruction =
            (instruction + 1);  // we'll resolve its address later
      } break;

      case Token_Identifier: {
        instruction = assembler.NewInstruction();
        uint supported_addressing = AM_IMPLIED;

        // Parse mnemonic
        if (token->Equals("lda")) {
          instruction->type = I_LDA;
          supported_addressing = AM_ALL;
        } else if (token->Equals("sta")) {
          instruction->type = I_STA;
          supported_addressing = AM_ALL;
        } else if (token->Equals("nop")) {
          instruction->type = I_NOP;
          supported_addressing = AM_IMPLIED;
        } else if (token->Equals("jmp")) {
          instruction->type = I_JMP;
          supported_addressing = AM_ABSOLUTE | AM_INDIRECT;
        } else {
          assembler.SyntaxError("Unknown command");
        }

        // Parse operand
        token = assembler.NextToken();

        if (supported_addressing & AM_IMMEDIATE) {
          if (token->type == Token_Hash) {
            token = assembler.PeekToken();
            if (token->type == Token_Identifier) {
              instruction->deferred_operand = assembler.NextToken();
            } else {
              instruction->operand = assembler.RequireNumber();
            }
          }
        }


        // instruction->type = I_LDA;
        // token = assembler.NextToken();
        // if (token->type == Token_Hash) {
        //   instruction->mode = AM_IMMEDIATE;
        //   instruction->operand = assembler.RequireNumber();

        // } else if (token->type == Token_Identifier) {
        //   // TODO: symbol table
        //   assembler.SyntaxError("not implemented");
        //   assembler.NextToken();
        // } else {
        //   assembler.PrevToken();
        //   instruction->operand = assembler.RequireNumber();
        //   instruction->mode = AM_ABSOLUTE;
        // }
        // } else if (token->Equals("sta")) {
        //   instruction->type = I_STA;
        //   token = assembler.PeekToken();
        //   if (token->type == Token_Identifier) {
        //     // TODO: symbol table
        //     assembler.SyntaxError("not implemented");
        //     assembler.NextToken();
        //   } else {
        //     instruction->operand = assembler.RequireNumber();
        //     instruction->mode = AM_ABSOLUTE;
        //   }
        // } else if (token->Equals("nop")) {
        //   // a command for lazy cpus
        //   instruction->type = I_NOP;
        // } else if (token->Equals("jmp")) {
        //   instruction->type = I_JMP;
        //   token = assembler.PeekToken();
        //   if (token->type == Token_Identifier) {
        //     token = assembler.NextToken();
        //     SymbolTableEntry *entry = symbol_table.GetEntry(token);
        //     if (entry == NULL) {
        //       instruction->deferred_operand = token;
        //     } else {
        //       instruction->operand = entry->value;
        //     }
        //   } else {
        //     instruction->operand = assembler.RequireNumber();
        //   }
        // } else {
        //   assembler.SyntaxError("Unknown command");
        // }
      } break;

      default: {
        assembler.SyntaxError("Command or label expected, got");
      } break;
    }
    token = assembler.NextToken();
  }

  CodeGenerator codegen = {};
  codegen.at_memory = (u8 *)gMachineMemory + memory_address;
  codegen.at_instruction = assembler.instructions;

  for (int i = 0; i < assembler.num_instructions; i++) {
  }

  print("Assembling of %s finished\n", filename);

  return 0;
}
