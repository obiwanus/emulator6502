// ================== Assembler ====================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "utils.cpp"

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
  bool IsNumber();
  int ParseNumber();
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

// Addressing mode flags
#define AMF_NONE 0x00
#define AMF_IMMEDIATE 0x01
#define AMF_ABSOLUTE 0x02
#define AMF_ABSOLUTE_X 0x04
#define AMF_ABSOLUTE_Y 0x08
#define AMF_ZERO_PAGE 0x20
#define AMF_ZERO_PAGE_X 0x40
#define AMF_ZERO_PAGE_Y 0x80
#define AMF_INDIRECT_Y 0x100
#define AMF_INDIRECT_X 0x200
#define AMF_MOST_COMMON 0x3FF

// Combined absolute + zero page
#define AMF_ABSOLUTE_Z 0x22
#define AMF_ABSOLUTE_ZX 0x44
#define AMF_ABSOLUTE_ZY 0x88

// not included in AMF_MOST_COMMON
#define AMF_INDIRECT 0x400
#define AMF_IMPLIED 0x800
#define AMF_ACCUMULATOR 0x1000
#define AMF_RELATIVE 0x2000

enum AddressingMode {
  AM_Unknown = 0,
  AM_Immediate,
  AM_Absolute,
  AM_Relative,
  AM_Absolute_X,
  AM_Absolute_Y,
  AM_Zeropage,
  AM_Zeropage_X,
  AM_Zeropage_Y,
  AM_Indirect_X,
  AM_Indirect_Y,
  AM_Indirect,
  AM_Implied,
  AM_Accumulator,
};

enum InstructionType {
  I_Unknown = 0,
  I_NOP,
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
  I_BVS,
  I_CLC,
  I_CLD,
  I_CLI,
  I_CLV,
  I_CMP,
  I_CPX,
  I_CPY,
  I_DEC,
  I_DEX,
  I_DEY,
  I_EOR,
  I_INC,
  I_INX,
  I_INY,
  I_JMP,
  I_JSR,
  I_LDA,
  I_LDX,
  I_LDY,
  I_LSR,
  I_ORA,
  I_PHA,
  I_PHP,
  I_PLA,
  I_PLP,
  I_ROL,
  I_ROR,
  I_RTI,
  I_RTS,
  I_SBC,
  I_SEC,
  I_SED,
  I_SEI,
  I_STA,
  I_STX,
  I_STY,
  I_TAX,
  I_TAY,
  I_TSX,
  I_TXA,
  I_TXS,
  I_TYA,
};

struct Instruction {
  InstructionType type;
  AddressingMode mode;
  u32 modes;  // supported addressing modes
  int operand;
  Token *deferred_operand;  // to be looked up in the symbol table
  Token *mnemonic;          // for error reporting
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
    if (string[i] == '\0' || toupper(string[i]) != toupper(this->text[i])) {
      return false;
    }
  }
  return true;
}

bool Token::IsNumber() {
  return this->type == Token_DecNumber || this->type == Token_HexNumber;
}

int Token::ParseNumber() {
  int value = 0;

  if (!this->IsNumber()) {
    this->SyntaxError("Trying to parse this as a number");
  }
  if (this->type == Token_HexNumber) {
    value = strtoi(this->text, this->length, 16);
  } else {
    value = strtoi(this->text, this->length, 10);
  }

  return value;
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

int Assembler::RequireNumber() {
  int value = 0;
  Token *token = this->NextToken();

  if (token->type != Token_HexNumber && token->type != Token_DecNumber) {
    this->SyntaxError("number expected, got");
  }

  value = token->ParseNumber();

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

int BytesForAddressingMode(AddressingMode mode) {
  int bytes = 0;

  if (mode == AM_Immediate)
    bytes = 2;
  else if (mode == AM_Zeropage)
    bytes = 2;
  else if (mode == AM_Zeropage_X)
    bytes = 2;
  else if (mode == AM_Absolute)
    bytes = 3;
  else if (mode == AM_Absolute_X)
    bytes = 3;
  else if (mode == AM_Absolute_Y)
    bytes = 3;
  else if (mode == AM_Indirect_X)
    bytes = 2;
  else if (mode == AM_Indirect_Y)
    bytes = 2;
  else if (mode == AM_Implied)
    bytes = 1;
  else if (mode == AM_Relative)
    bytes = 2;
  else if (mode == AM_Accumulator)
    bytes = 1;
  else if (mode == AM_Indirect)
    bytes = 3;

  return bytes;
}

Token *GetToken(Tokenizer *tokenizer) {
  EatWhiteSpace(tokenizer);

  Token *token = tokenizer->NewToken();
  token->text = tokenizer->at;
  token->length = 0;
  token->line_num = tokenizer->line_num;  // for error reporting

  char c = *tokenizer->at;
  if (IsAlpha(c)) {
    while (!IsWhitespace(c) && c != ':' && c != '\0' && c != '(' && c != ')' &&
           c != ',') {
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

static int LoadProgram(char *filename, u16 memory_address) {
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
        instruction->mnemonic = token;

        // Parse mnemonic
        if (token->Equals("ADC")) {
          instruction->type = I_ADC;
          instruction->modes = AMF_MOST_COMMON;
        } else if (token->Equals("AND")) {
          instruction->type = I_AND;
          instruction->modes = AMF_MOST_COMMON;
        } else if (token->Equals("ASL")) {
          instruction->type = I_ASL;
          instruction->modes = AMF_ACCUMULATOR | AMF_ABSOLUTE_Z | AMF_ABSOLUTE_ZX;
        } else if (token->Equals("BCC")) {
          instruction->type = I_BCC;
          instruction->modes = AMF_RELATIVE;
        } else if (token->Equals("BCS")) {
          instruction->type = I_BCS;
          instruction->modes = AMF_RELATIVE;
        } else if (token->Equals("BEQ")) {
          instruction->type = I_BEQ;
          instruction->modes = AMF_RELATIVE;
        } else if (token->Equals("BIT")) {
          instruction->type = I_BIT;
          instruction->modes = AMF_ABSOLUTE_Z;
        } else if (token->Equals("BMI")) {
          instruction->type = I_BMI;
          instruction->modes = AMF_RELATIVE;
        } else if (token->Equals("BNE")) {
          instruction->type = I_BNE;
          instruction->modes = AMF_RELATIVE;
        } else if (token->Equals("BPL")) {
          instruction->type = I_BPL;
          instruction->modes = AMF_RELATIVE;
        } else if (token->Equals("BRK")) {
          instruction->type = I_BRK;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("BVC")) {
          instruction->type = I_BVC;
          instruction->modes = AMF_RELATIVE;
        } else if (token->Equals("BVS")) {
          instruction->type = I_BVS;
          instruction->modes = AMF_RELATIVE;
        } else if (token->Equals("CLC")) {
          instruction->type = I_CLC;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("CLD")) {
          instruction->type = I_CLD;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("CLI")) {
          instruction->type = I_CLI;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("CLV")) {
          instruction->type = I_CLV;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("CMP")) {
          instruction->type = I_CMP;
          instruction->modes = AMF_MOST_COMMON;
        } else if (token->Equals("CPX")) {
          instruction->type = I_CPX;
          instruction->modes = AMF_IMMEDIATE | AMF_ABSOLUTE_Z;
        } else if (token->Equals("CPY")) {
          instruction->type = I_CPY;
          instruction->modes = AMF_IMMEDIATE | AMF_ABSOLUTE_Z;
        } else if (token->Equals("DEC")) {
          instruction->type = I_DEC;
          instruction->modes = AMF_ABSOLUTE_Z | AMF_ABSOLUTE_ZX;
        } else if (token->Equals("DEX")) {
          instruction->type = I_DEX;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("DEY")) {
          instruction->type = I_DEY;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("EOR")) {
          instruction->type = I_EOR;
          instruction->modes = AMF_MOST_COMMON;
        } else if (token->Equals("INC")) {
          instruction->type = I_INC;
          instruction->modes = AMF_ABSOLUTE_Z | AMF_ABSOLUTE_ZX;
        } else if (token->Equals("INX")) {
          instruction->type = I_INX;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("INY")) {
          instruction->type = I_INY;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("JMP")) {
          instruction->type = I_JMP;
          instruction->modes = AMF_ABSOLUTE | AMF_INDIRECT;
        } else if (token->Equals("JSR")) {
          instruction->type = I_JSR;
          instruction->modes = AMF_ABSOLUTE;
        } else if (token->Equals("LDA")) {
          instruction->type = I_LDA;
          instruction->modes = AMF_MOST_COMMON;
        } else if (token->Equals("LDX")) {
          instruction->type = I_LDX;
          instruction->modes = AMF_IMMEDIATE | AMF_ABSOLUTE_Z | AMF_ABSOLUTE_ZY;
        } else if (token->Equals("LDY")) {
          instruction->type = I_LDY;
          instruction->modes = AMF_IMMEDIATE | AMF_ABSOLUTE_Z | AMF_ABSOLUTE_ZX;
        } else if (token->Equals("LSR")) {
          instruction->type = I_LSR;
          instruction->modes = AMF_ACCUMULATOR | AMF_ABSOLUTE_Z | AMF_ABSOLUTE_ZX;
        } else if (token->Equals("NOP")) {
          instruction->type = I_NOP;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("ORA")) {
          instruction->type = I_ORA;
          instruction->modes = AMF_MOST_COMMON;
        } else if (token->Equals("PHA")) {
          instruction->type = I_PHA;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("PHP")) {
          instruction->type = I_PHP;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("PLA")) {
          instruction->type = I_PLA;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("PLP")) {
          instruction->type = I_PLP;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("ROL")) {
          instruction->type = I_ROL;
          instruction->modes = AMF_ACCUMULATOR | AMF_ABSOLUTE_Z | AMF_ABSOLUTE_ZX;
        } else if (token->Equals("ROR")) {
          instruction->type = I_ROR;
          instruction->modes = AMF_ACCUMULATOR | AMF_ABSOLUTE_Z | AMF_ABSOLUTE_ZX;
        } else if (token->Equals("RTI")) {
          instruction->type = I_RTI;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("RTS")) {
          instruction->type = I_RTS;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("SBC")) {
          instruction->type = I_SBC;
          instruction->modes = AMF_MOST_COMMON;
        } else if (token->Equals("SEC")) {
          instruction->type = I_SEC;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("SED")) {
          instruction->type = I_SED;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("SEI")) {
          instruction->type = I_SEI;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("STA")) {
          instruction->type = I_STA;
          instruction->modes = AMF_MOST_COMMON & ~AMF_IMMEDIATE;
        } else if (token->Equals("STX")) {
          instruction->type = I_STX;
          instruction->modes = AMF_ABSOLUTE_Z | AMF_ZERO_PAGE_Y;
        } else if (token->Equals("STY")) {
          instruction->type = I_STY;
          instruction->modes = AMF_ABSOLUTE_Z | AMF_ZERO_PAGE_X;
        } else if (token->Equals("TAX")) {
          instruction->type = I_TAX;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("TAY")) {
          instruction->type = I_TAY;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("TSX")) {
          instruction->type = I_TSX;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("TXA")) {
          instruction->type = I_TXA;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("TXS")) {
          instruction->type = I_TXS;
          instruction->modes = AMF_IMPLIED;
        } else if (token->Equals("TYA")) {
          instruction->type = I_TYA;
          instruction->modes = AMF_MOST_COMMON;
        } else {
          assembler.SyntaxError("Unknown command");
        }

        u32 modes = instruction->modes;

        if (modes & AMF_IMPLIED) {
          // No operand required
          instruction->mode = AM_Implied;
          break;
        }

        // Parse operand
        token = assembler.NextToken();

        if ((modes & AMF_IMMEDIATE) && token->type == Token_Hash) {
          token = assembler.PeekToken();
          if (token->type == Token_Identifier) {
            instruction->deferred_operand = assembler.NextToken();
          } else {
            instruction->operand = assembler.RequireNumber();
          }
          instruction->mode = AM_Immediate;
          break;
        }

        if ((modes & (AMF_ABSOLUTE | AMF_ABSOLUTE_X | AMF_ABSOLUTE_Y |
                      AMF_ZERO_PAGE | AMF_ZERO_PAGE_X | AMF_ZERO_PAGE_Y)) &&
            (token->type == Token_Identifier || token->IsNumber())) {
          if (token->type == Token_Identifier) {
            instruction->deferred_operand = token;
          } else {
            instruction->operand = token->ParseNumber();
          }
          if (assembler.PeekToken()->type == Token_Comma) {
            assembler.NextToken();  // skip the comma
            token = assembler.NextToken();
            if (token->Equals("x")) {
              instruction->mode = AM_Absolute_X;
            } else if (token->Equals("y")) {
              instruction->mode = AM_Absolute_Y;
            } else {
              token->SyntaxError("X or Y expected, got");
            }
          } else {
            instruction->mode = AM_Absolute;
          }
          break;
        }

        if ((modes & (AMF_INDIRECT_X | AMF_INDIRECT_Y | AMF_INDIRECT)) &&
            token->type == Token_OpenParen) {
          token = assembler.NextToken();
          if (token->type == Token_Identifier) {
            instruction->deferred_operand = token;
          } else {
            assembler.PrevToken();
            instruction->operand = assembler.RequireNumber();
          }
          if (modes & AMF_INDIRECT) {
            assembler.RequireToken(Token_CloseParen);
            instruction->mode = AM_Indirect;
          } else {
            token = assembler.NextToken();
            if (token->type == Token_CloseParen && (modes & AMF_INDIRECT_Y)) {
              assembler.RequireToken(Token_Comma);
              token = assembler.NextToken();
              if (token->Equals("y")) {
                instruction->mode = AM_Indirect_Y;
              } else {
                token->SyntaxError("Y expected, got");
              }
            } else if (token->type == Token_Comma && (modes & AMF_INDIRECT_X)) {
              token = assembler.NextToken();
              if (token->Equals("X")) {
                instruction->mode = AM_Indirect_X;
              } else {
                token->SyntaxError("X expected, got");
              }
              assembler.RequireToken(Token_CloseParen);
            } else {
              token->SyntaxError(") or , expected, got");
            }
          }
          break;
        }

        if ((modes & AMF_RELATIVE) && token->type == Token_Identifier) {
          // We don't support offsets (yet?), only labels
          instruction->deferred_operand = token;
          instruction->mode = AM_Relative;
          break;
        }

        if ((modes & AMF_ACCUMULATOR) && token->type == Token_Identifier &&
            token->Equals("A")) {
          instruction->mode = AM_Accumulator;
          break;
        }

        // Couldn't match operand
        token->SyntaxError("Incorrect operand");
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

  u8 *memory = (u8 *)gMachineMemory + memory_address;  // for debugging

  for (int i = 0; i < assembler.num_instructions; i++) {
    instruction = assembler.instructions + i;

    // Resolve deferred operand
    if (instruction->deferred_operand != NULL) {
      SymbolTableEntry *entry =
          symbol_table.GetEntry(instruction->deferred_operand);
      if (entry == NULL) {
        instruction->deferred_operand->SyntaxError("Unknown identifier");
      } else {
        instruction->operand = entry->value;
      }
    }

    if ((instruction->mode == AMF_ABSOLUTE &&
         !(instruction->modes & AMF_ABSOLUTE)) ||
        (instruction->mode == AMF_ABSOLUTE_X &&
         !(instruction->modes & AMF_ABSOLUTE_X)) ||
        (instruction->mode == AMF_ABSOLUTE_Y &&
         !(instruction->modes & AMF_ABSOLUTE_Y))) {
      if (instruction->operand > 0xFF) {
        instruction->mnemonic->SyntaxError(
            "Only zero page addressing is supported");
      }
    }

    if (instruction->mode == AMF_ABSOLUTE &&
        (instruction->modes & AMF_ZERO_PAGE) && instruction->operand <= 0xFF) {
      instruction->mode = AM_Zeropage;
    }
    if (instruction->mode == AMF_ABSOLUTE_X &&
        (instruction->modes & AMF_ZERO_PAGE_X) &&
        instruction->operand <= 0xFF) {
      instruction->mode = AM_Zeropage_X;
    }
    if (instruction->mode == AMF_ABSOLUTE_Y &&
        (instruction->modes & AMF_ZERO_PAGE_Y) &&
        instruction->operand <= 0xFF) {
      instruction->mode = AM_Zeropage_Y;
    }

    u8 opcode = 0;
    AddressingMode mode = instruction->mode;
    InstructionType type = instruction->type;
    bool error = false;

    switch (type) {
      case I_ADC: {
        if (mode == AM_Immediate)
          opcode = 0x69;
        else if (mode == AM_Zeropage)
          opcode = 0x65;
        else if (mode == AM_Zeropage_X)
          opcode = 0x75;
        else if (mode == AM_Absolute)
          opcode = 0x6D;
        else if (mode == AM_Absolute_X)
          opcode = 0x7D;
        else if (mode == AM_Absolute_Y)
          opcode = 0x79;
        else if (mode == AM_Indirect_X)
          opcode = 0x61;
        else if (mode == AM_Indirect_Y)
          opcode = 0x71;
        else
          error = true;
      } break;
      case I_AND: {
        if (mode == AM_Immediate)
          opcode = 0x29;
        else if (mode == AM_Zeropage)
          opcode = 0x25;
        else if (mode == AM_Zeropage_X)
          opcode = 0x35;
        else if (mode == AM_Absolute)
          opcode = 0x2D;
        else if (mode == AM_Absolute_X)
          opcode = 0x3D;
        else if (mode == AM_Absolute_Y)
          opcode = 0x39;
        else if (mode == AM_Indirect_X)
          opcode = 0x21;
        else if (mode == AM_Indirect_Y)
          opcode = 0x31;
        else
          error = true;
      } break;
      case I_ASL: {
        if (mode == AM_Accumulator)
          opcode = 0x0A;
        else if (mode == AM_Zeropage)
          opcode = 0x06;
        else if (mode == AM_Zeropage_X)
          opcode = 0x16;
        else if (mode == AM_Absolute)
          opcode = 0x0E;
        else if (mode == AM_Absolute_X)
          opcode = 0x1E;
        else
          error = true;
      } break;
      case I_BCC: {
        if (mode == AM_Relative)
          opcode = 0x90;
        else
          error = true;
      } break;
      case I_BCS: {
        if (mode == AM_Relative)
          opcode = 0xB0;
        else
          error = true;
      } break;
      case I_LDA: {
        if (mode == AM_Immediate)
          opcode = 0xA9;
        else if (mode == AM_Zeropage)
          opcode = 0xA5;
        else if (mode == AM_Zeropage_X)
          opcode = 0xB5;
        else if (mode == AM_Absolute)
          opcode = 0xAD;
        else if (mode == AM_Absolute_X)
          opcode = 0xBD;
        else if (mode == AM_Absolute_Y)
          opcode = 0xB9;
        else if (mode == AM_Indirect_X)
          opcode = 0xA1;
        else if (mode == AM_Indirect_Y)
          opcode = 0xB1;
        else
          error = true;
      } break;
      case I_STA: {
        if (mode == AM_Zeropage)
          opcode = 0x85;
        else if (mode == AM_Zeropage_X)
          opcode = 0x95;
        else if (mode == AM_Absolute)
          opcode = 0x8D;
        else if (mode == AM_Absolute_X)
          opcode = 0x9D;
        else if (mode == AM_Absolute_Y)
          opcode = 0x99;
        else if (mode == AM_Indirect_X)
          opcode = 0x81;
        else if (mode == AM_Indirect_Y)
          opcode = 0x91;
        else
          error = true;
      } break;
      case I_JMP: {
        if (mode == AM_Absolute)
          opcode = 0x4C;
        else if (mode == AM_Indirect)
          opcode = 0x6C;
        else
          error = true;
      } break;
    }

    // A separate block for the single byte instructions
    if (mode == AM_Implied) {
      if (type == I_NOP)
        opcode = 0xEA;
      else if (type == I_INX)
        opcode = 0xE8;
      else
        error = true;
    }

    int bytes = BytesForAddressingMode(mode);

    if (error || bytes == 0) {
      instruction->mnemonic->SyntaxError("Incorrect addressing mode");
    }

    if ((bytes == 2 && instruction->operand > 0xFF) ||
        (bytes == 3 && instruction->operand > 0xFFFF)) {
      instruction->mnemonic->SyntaxError(
          "Operand is too large for this addressing mode");
    }

    // Finally, write in memory
    *codegen.at_memory = opcode;
    codegen.at_memory++;
    if (bytes == 2) {
      *codegen.at_memory = (u8)instruction->operand;
      codegen.at_memory++;
    } else if (bytes == 3) {
      u8 lsb = (u8)instruction->operand;
      u8 msb = (u8)(instruction->operand >> 8);
      *codegen.at_memory = lsb;
      codegen.at_memory++;
      *codegen.at_memory = msb;
      codegen.at_memory++;
    }
  }

  print("Assembling of %s finished\n", filename);

  return 0;
}
