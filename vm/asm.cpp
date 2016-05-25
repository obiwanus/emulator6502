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
  Token_Define,

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

  I_END,  // non-canonical
};

global int gBytesForAddressingMode[AM_Accumulator + 1] = {
    0,  // AM_Unknown
    2,  // AM_Immediate
    3,  // AM_Absolute
    3,  // AM_Relative
    3,  // AM_Absolute_X
    3,  // AM_Absolute_Y
    2,  // AM_Zeropage
    2,  // AM_Zeropage_X
    2,  // AM_Zeropage_Y
    2,  // AM_Indirect_X
    2,  // AM_Indirect_Y
    3,  // AM_Indirect
    1,  // AM_Implied
    1,  // AM_Accumulator
};

struct InstructionTypeAndMode {
  InstructionType type;
  AddressingMode mode;
};

const InstructionTypeAndMode gOpcodeToInstruction[256] = {
    {I_BRK, AM_Implied},
    {I_ORA, AM_Indirect_X},
    {},
    {},
    {},
    {I_ORA, AM_Zeropage},
    {I_ASL, AM_Zeropage},
    {},
    {I_PHP, AM_Implied},
    {I_ORA, AM_Immediate},
    {I_ASL, AM_Accumulator},
    {},
    {},
    {I_ORA, AM_Absolute},
    {I_ASL, AM_Absolute},
    {},
    {I_BPL, AM_Relative},
    {I_ORA, AM_Indirect_Y},
    {},
    {},
    {},
    {I_ORA, AM_Zeropage_X},
    {I_ASL, AM_Zeropage_X},
    {},
    {I_CLC, AM_Implied},
    {I_ORA, AM_Absolute_Y},
    {},
    {},
    {},
    {I_ORA, AM_Absolute_X},
    {I_ASL, AM_Absolute_X},
    {},
    {I_JSR, AM_Implied},
    {I_AND, AM_Indirect_X},
    {},
    {},
    {I_BIT, AM_Zeropage},
    {I_AND, AM_Zeropage},
    {I_ROL, AM_Zeropage},
    {},
    {I_PLP, AM_Implied},
    {I_AND, AM_Immediate},
    {I_ROL, AM_Accumulator},
    {},
    {I_BIT, AM_Absolute},
    {I_AND, AM_Absolute},
    {I_ROL, AM_Absolute},
    {},
    {I_BMI, AM_Relative},
    {I_AND, AM_Indirect_Y},
    {},
    {},
    {},
    {I_AND, AM_Zeropage_X},
    {I_ROL, AM_Zeropage_X},
    {},
    {I_SEC, AM_Implied},
    {I_AND, AM_Absolute_Y},
    {},
    {},
    {},
    {I_AND, AM_Absolute_X},
    {I_ROL, AM_Absolute_X},
    {},
    {I_RTI, AM_Implied},
    {I_EOR, AM_Indirect_X},
    {},
    {},
    {},
    {I_EOR, AM_Zeropage},
    {I_LSR, AM_Zeropage},
    {},
    {I_PHA, AM_Implied},
    {I_EOR, AM_Immediate},
    {I_LSR, AM_Accumulator},
    {},
    {I_JMP, AM_Absolute},
    {I_EOR, AM_Absolute},
    {I_LSR, AM_Absolute},
    {},
    {I_BVC, AM_Relative},
    {I_EOR, AM_Indirect_Y},
    {},
    {},
    {},
    {I_EOR, AM_Zeropage_X},
    {I_LSR, AM_Zeropage_X},
    {},
    {I_CLI, AM_Implied},
    {I_EOR, AM_Absolute_Y},
    {},
    {},
    {},
    {I_EOR, AM_Absolute_X},
    {I_LSR, AM_Absolute_X},
    {},
    {I_RTS, AM_Implied},
    {I_ADC, AM_Indirect_X},
    {},
    {},
    {},
    {I_ADC, AM_Zeropage},
    {I_ROR, AM_Zeropage},
    {},
    {I_PLA, AM_Implied},
    {I_ADC, AM_Immediate},
    {I_ROR, AM_Accumulator},
    {},
    {I_JMP, AM_Indirect},
    {I_ADC, AM_Absolute},
    {I_ROR, AM_Absolute},
    {},
    {I_BVS, AM_Relative},
    {I_ADC, AM_Indirect_Y},
    {},
    {},
    {},
    {I_ADC, AM_Zeropage_X},
    {I_ROR, AM_Zeropage_X},
    {},
    {I_SEI, AM_Implied},
    {I_ADC, AM_Absolute_Y},
    {},
    {},
    {},
    {I_ADC, AM_Absolute_X},
    {I_ROR, AM_Absolute_X},
    {},
    {},
    {I_STA, AM_Indirect_X},
    {},
    {},
    {I_STY, AM_Zeropage},
    {I_STA, AM_Zeropage},
    {I_STX, AM_Zeropage},
    {},
    {I_DEY, AM_Implied},
    {},
    {I_TXA, AM_Implied},
    {},
    {I_STY, AM_Absolute},
    {I_STA, AM_Absolute},
    {I_STX, AM_Absolute},
    {},
    {I_BCC, AM_Relative},
    {I_STA, AM_Indirect_Y},
    {},
    {},
    {I_STY, AM_Zeropage_X},
    {I_STA, AM_Zeropage_X},
    {I_STX, AM_Zeropage_Y},
    {},
    {I_TYA, AM_Implied},
    {I_STA, AM_Absolute_Y},
    {I_TXS, AM_Implied},
    {},
    {},
    {I_STA, AM_Absolute_X},
    {},
    {},
    {I_LDY, AM_Immediate},
    {I_LDA, AM_Indirect_X},
    {I_LDX, AM_Immediate},
    {},
    {I_LDY, AM_Zeropage},
    {I_LDA, AM_Zeropage},
    {I_LDX, AM_Zeropage},
    {},
    {I_TAY, AM_Implied},
    {I_LDA, AM_Immediate},
    {I_TAX, AM_Implied},
    {},
    {I_LDY, AM_Absolute},
    {I_LDA, AM_Absolute},
    {I_LDX, AM_Absolute},
    {},
    {I_BCS, AM_Relative},
    {I_LDA, AM_Indirect_Y},
    {},
    {},
    {I_LDY, AM_Zeropage_X},
    {I_LDA, AM_Zeropage_X},
    {I_LDX, AM_Zeropage_Y},
    {},
    {I_CLV, AM_Implied},
    {I_LDA, AM_Absolute_Y},
    {I_TSX, AM_Implied},
    {},
    {I_LDY, AM_Absolute_X},
    {I_LDA, AM_Absolute_X},
    {I_LDX, AM_Absolute_Y},
    {},
    {I_CPY, AM_Immediate},
    {I_CMP, AM_Indirect_X},
    {},
    {},
    {I_CPY, AM_Zeropage},
    {I_CMP, AM_Zeropage},
    {I_DEC, AM_Zeropage},
    {},
    {I_INY, AM_Implied},
    {I_CMP, AM_Immediate},
    {I_DEX, AM_Implied},
    {},
    {I_CPY, AM_Absolute},
    {I_CMP, AM_Absolute},
    {I_DEC, AM_Absolute},
    {},
    {I_BNE, AM_Relative},
    {I_CMP, AM_Indirect_Y},
    {},
    {},
    {},
    {I_CMP, AM_Zeropage_X},
    {I_DEC, AM_Zeropage_X},
    {},
    {I_CLD, AM_Implied},
    {I_CMP, AM_Absolute_Y},
    {},
    {},
    {},
    {I_CMP, AM_Absolute_X},
    {I_DEC, AM_Absolute_X},
    {},
    {I_CPX, AM_Immediate},
    {I_SBC, AM_Indirect_X},
    {},
    {},
    {I_CPX, AM_Zeropage},
    {I_SBC, AM_Zeropage},
    {I_INC, AM_Zeropage},
    {},
    {I_INX, AM_Implied},
    {I_SBC, AM_Immediate},
    {I_NOP, AM_Implied},
    {},
    {I_CPX, AM_Absolute},
    {I_SBC, AM_Absolute},
    {I_INC, AM_Absolute},
    {},
    {I_BEQ, AM_Relative},
    {I_SBC, AM_Indirect_Y},
    {},
    {},
    {},
    {I_SBC, AM_Zeropage_X},
    {I_INC, AM_Zeropage_X},
    {},
    {I_SED, AM_Implied},
    {I_SBC, AM_Absolute_Y},
    {},
    {},
    {},
    {I_SBC, AM_Absolute_X},
    {I_INC, AM_Absolute_X},
    {I_END, AM_Implied},
};

// Filled in later using the mapping above
global u8 gInstructionToOpcode[I_END + 1][AM_Accumulator + 1] = {};

struct Instruction {
  InstructionType type;
  AddressingMode mode;
  u32 modes;  // supported addressing modes
  int operand;
  Token *deferred_operand;  // to be looked up in the symbol table
  Token *mnemonic;          // for error reporting
  int address;              // for labels. filled at a later stage
};

struct SymbolTableEntry {
  char *symbol;
  int value;
  Instruction *instruction;  // the instruction immediately following the label
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

static int LoadProgram(char *filename, u16 memory_address) {
  char *file_contents = ReadFileIntoString(filename);
  print("Assembling %s ...\n", filename);

  // ****************************************************************
  // Tokenizing
  // ****************************************************************
  Tokenizer tokenizer = {};
  tokenizer.at = file_contents;
  tokenizer.line_num = 1;

  bool tokenizing = true;
  while (tokenizing) {
    // Eat white space
    for (;;) {
      char c = tokenizer.at[0];
      if (IsWhitespace(c)) {
        if (c == '\n') {
          tokenizer.line_num++;
        }
        tokenizer.at++;
      } else if (c == '/' && tokenizer.at[1] == '/') {
        tokenizer.at += 2;
        while (!IsEndOfLine(*tokenizer.at)) {
          tokenizer.at++;
        }
      } else {
        break;
      }
    }

    // Get token
    Token *token = tokenizer.NewToken();
    token->text = tokenizer.at;
    token->length = 0;
    token->line_num = tokenizer.line_num;  // for error reporting

    char c = *tokenizer.at;
    if (IsAlpha(c)) {
      while (!IsWhitespace(c) && c != ':' && c != '\0' && c != '(' &&
             c != ')' && c != ',') {
        c = *++tokenizer.at;
        token->length++;
      }
      if (c == ':') {
        token->type = Token_Label;
        tokenizer.at++;
      } else {
        token->type = Token_Identifier;
        if (token->Equals("DEFINE")) {
          token->type = Token_Define;
        }
      }
    } else if (c == '#') {
      token->type = Token_Hash;
      token->length = 1;
      tokenizer.at++;
    } else if (c == '(') {
      token->type = Token_OpenParen;
      token->length = 1;
      tokenizer.at++;
    } else if (c == ')') {
      token->type = Token_CloseParen;
      token->length = 1;
      tokenizer.at++;
    } else if (c == ',') {
      token->type = Token_Comma;
      token->length = 1;
      tokenizer.at++;
    } else if (IsDecimal(c)) {
      token->type = Token_DecNumber;
      while (IsDecimal(*tokenizer.at)) {
        tokenizer.at++;
        token->length++;
      }
    } else if (c == '$') {
      token->type = Token_HexNumber;
      tokenizer.at++;
      token->text++;  // skip the $
      while (IsHex(*tokenizer.at)) {
        tokenizer.at++;
        token->length++;
      }
      if (token->length == 0) {
        token->type = Token_SyntaxError;
        sprintf(tokenizer.error_msg, "Number expected after $");
      }
    } else if (c == '\0') {
      token->type = Token_EndOfStream;
    } else {
      token->type = Token_SyntaxError;
      while (!IsWhitespace(c)) {
        c = *++tokenizer.at;
        token->length++;
      }
      sprintf(tokenizer.error_msg, "Unknown token '%.*s'", token->length,
              token->text);
    }
    // End get token

    if (token->type == Token_EndOfStream) {
      tokenizing = false;
    } else if (token->type == Token_SyntaxError) {
      print("Syntax error at line %d: %s\n", token->line_num,
            tokenizer.error_msg);
      exit(1);
    }
  }

  // ****************************************************************
  // Parsing
  // ****************************************************************

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

      case Token_Define: {
        token = assembler.RequireToken(Token_Identifier);
        int value = assembler.RequireNumber();
        symbol_table.AddEntry(token, value);
      } break;

      case Token_Identifier: {
        instruction = assembler.NewInstruction();
        instruction->mnemonic = token;
        u32 modes = AMF_NONE;
        InstructionType type = I_Unknown;

        // Parse mnemonic
        if (token->Equals("ADC")) {
          type = I_ADC;
          modes = AMF_MOST_COMMON;
        } else if (token->Equals("AND")) {
          type = I_AND;
          modes = AMF_MOST_COMMON;
        } else if (token->Equals("ASL")) {
          type = I_ASL;
          modes = AMF_ACCUMULATOR | AMF_ABSOLUTE_Z | AMF_ABSOLUTE_ZX;
        } else if (token->Equals("BCC")) {
          type = I_BCC;
          modes = AMF_RELATIVE;
        } else if (token->Equals("BCS")) {
          type = I_BCS;
          modes = AMF_RELATIVE;
        } else if (token->Equals("BEQ")) {
          type = I_BEQ;
          modes = AMF_RELATIVE;
        } else if (token->Equals("BIT")) {
          type = I_BIT;
          modes = AMF_ABSOLUTE_Z;
        } else if (token->Equals("BMI")) {
          type = I_BMI;
          modes = AMF_RELATIVE;
        } else if (token->Equals("BNE")) {
          type = I_BNE;
          modes = AMF_RELATIVE;
        } else if (token->Equals("BPL")) {
          type = I_BPL;
          modes = AMF_RELATIVE;
        } else if (token->Equals("BRK")) {
          type = I_BRK;
          modes = AMF_IMPLIED;
        } else if (token->Equals("BVC")) {
          type = I_BVC;
          modes = AMF_RELATIVE;
        } else if (token->Equals("BVS")) {
          type = I_BVS;
          modes = AMF_RELATIVE;
        } else if (token->Equals("CLC")) {
          type = I_CLC;
          modes = AMF_IMPLIED;
        } else if (token->Equals("CLD")) {
          type = I_CLD;
          modes = AMF_IMPLIED;
        } else if (token->Equals("CLI")) {
          type = I_CLI;
          modes = AMF_IMPLIED;
        } else if (token->Equals("CLV")) {
          type = I_CLV;
          modes = AMF_IMPLIED;
        } else if (token->Equals("CMP")) {
          type = I_CMP;
          modes = AMF_MOST_COMMON;
        } else if (token->Equals("CPX")) {
          type = I_CPX;
          modes = AMF_IMMEDIATE | AMF_ABSOLUTE_Z;
        } else if (token->Equals("CPY")) {
          type = I_CPY;
          modes = AMF_IMMEDIATE | AMF_ABSOLUTE_Z;
        } else if (token->Equals("DEC")) {
          type = I_DEC;
          modes = AMF_ABSOLUTE_Z | AMF_ABSOLUTE_ZX;
        } else if (token->Equals("DEX")) {
          type = I_DEX;
          modes = AMF_IMPLIED;
        } else if (token->Equals("DEY")) {
          type = I_DEY;
          modes = AMF_IMPLIED;
        } else if (token->Equals("EOR")) {
          type = I_EOR;
          modes = AMF_MOST_COMMON;
        } else if (token->Equals("INC")) {
          type = I_INC;
          modes = AMF_ABSOLUTE_Z | AMF_ABSOLUTE_ZX;
        } else if (token->Equals("INX")) {
          type = I_INX;
          modes = AMF_IMPLIED;
        } else if (token->Equals("INY")) {
          type = I_INY;
          modes = AMF_IMPLIED;
        } else if (token->Equals("JMP")) {
          type = I_JMP;
          modes = AMF_ABSOLUTE | AMF_INDIRECT;
        } else if (token->Equals("JSR")) {
          type = I_JSR;
          modes = AMF_ABSOLUTE;
        } else if (token->Equals("LDA")) {
          type = I_LDA;
          modes = AMF_MOST_COMMON;
        } else if (token->Equals("LDX")) {
          type = I_LDX;
          modes = AMF_IMMEDIATE | AMF_ABSOLUTE_Z | AMF_ABSOLUTE_ZY;
        } else if (token->Equals("LDY")) {
          type = I_LDY;
          modes = AMF_IMMEDIATE | AMF_ABSOLUTE_Z | AMF_ABSOLUTE_ZX;
        } else if (token->Equals("LSR")) {
          type = I_LSR;
          modes = AMF_ACCUMULATOR | AMF_ABSOLUTE_Z | AMF_ABSOLUTE_ZX;
        } else if (token->Equals("NOP")) {
          type = I_NOP;
          modes = AMF_IMPLIED;
        } else if (token->Equals("ORA")) {
          type = I_ORA;
          modes = AMF_MOST_COMMON;
        } else if (token->Equals("PHA")) {
          type = I_PHA;
          modes = AMF_IMPLIED;
        } else if (token->Equals("PHP")) {
          type = I_PHP;
          modes = AMF_IMPLIED;
        } else if (token->Equals("PLA")) {
          type = I_PLA;
          modes = AMF_IMPLIED;
        } else if (token->Equals("PLP")) {
          type = I_PLP;
          modes = AMF_IMPLIED;
        } else if (token->Equals("ROL")) {
          type = I_ROL;
          modes = AMF_ACCUMULATOR | AMF_ABSOLUTE_Z | AMF_ABSOLUTE_ZX;
        } else if (token->Equals("ROR")) {
          type = I_ROR;
          modes = AMF_ACCUMULATOR | AMF_ABSOLUTE_Z | AMF_ABSOLUTE_ZX;
        } else if (token->Equals("RTI")) {
          type = I_RTI;
          modes = AMF_IMPLIED;
        } else if (token->Equals("RTS")) {
          type = I_RTS;
          modes = AMF_IMPLIED;
        } else if (token->Equals("SBC")) {
          type = I_SBC;
          modes = AMF_MOST_COMMON;
        } else if (token->Equals("SEC")) {
          type = I_SEC;
          modes = AMF_IMPLIED;
        } else if (token->Equals("SED")) {
          type = I_SED;
          modes = AMF_IMPLIED;
        } else if (token->Equals("SEI")) {
          type = I_SEI;
          modes = AMF_IMPLIED;
        } else if (token->Equals("STA")) {
          type = I_STA;
          modes = AMF_MOST_COMMON & ~AMF_IMMEDIATE;
        } else if (token->Equals("STX")) {
          type = I_STX;
          modes = AMF_ABSOLUTE_Z | AMF_ZERO_PAGE_Y;
        } else if (token->Equals("STY")) {
          type = I_STY;
          modes = AMF_ABSOLUTE_Z | AMF_ZERO_PAGE_X;
        } else if (token->Equals("TAX")) {
          type = I_TAX;
          modes = AMF_IMPLIED;
        } else if (token->Equals("TAY")) {
          type = I_TAY;
          modes = AMF_IMPLIED;
        } else if (token->Equals("TSX")) {
          type = I_TSX;
          modes = AMF_IMPLIED;
        } else if (token->Equals("TXA")) {
          type = I_TXA;
          modes = AMF_IMPLIED;
        } else if (token->Equals("TXS")) {
          type = I_TXS;
          modes = AMF_IMPLIED;
        } else if (token->Equals("TYA")) {
          type = I_TYA;
          modes = AMF_MOST_COMMON;
        } else if (token->Equals("END")) {
          type = I_END;
          modes = AMF_IMPLIED;
        } else {
          assembler.SyntaxError("Unknown command");
        }

        instruction->type = type;
        instruction->modes = modes;

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

  // ****************************************************************
  // Code generation
  // ****************************************************************

  CodeGenerator codegen = {};
  codegen.at_memory = (u8 *)gMachineMemory + memory_address;
  codegen.at_instruction = assembler.instructions;

  u8 *memory = (u8 *)gMachineMemory + memory_address;  // for debugging

  // Fill in instruction addresses that are used in label resolving
  int address = memory_address;
  for (int i = 0; i < assembler.num_instructions; i++) {
    instruction = assembler.instructions + i;
    instruction->address = address;

    // Check for zero page - important for instruction size!
    if (instruction->deferred_operand == NULL && instruction->operand <= 0xFF) {
      // If it's a deferred operand it's NOT a zero page mode in our asm
      // TODO: think about that (when there's relative addressing supported)
      if (instruction->mode == AMF_ABSOLUTE &&
          (instruction->modes & AMF_ZERO_PAGE)) {
        instruction->mode = AM_Zeropage;
      }
      if (instruction->mode == AMF_ABSOLUTE_X &&
          (instruction->modes & AMF_ZERO_PAGE_X)) {
        instruction->mode = AM_Zeropage_X;
      }
      if (instruction->mode == AMF_ABSOLUTE_Y &&
          (instruction->modes & AMF_ZERO_PAGE_Y)) {
        instruction->mode = AM_Zeropage_Y;
      }
    }

    int bytes = gBytesForAddressingMode[instruction->mode];
    address += bytes;
  }

  for (int i = 0; i < assembler.num_instructions; i++) {
    instruction = assembler.instructions + i;

    // Resolve deferred operand
    if (instruction->deferred_operand != NULL) {
      SymbolTableEntry *entry =
          symbol_table.GetEntry(instruction->deferred_operand);
      if (entry == NULL) {
        instruction->deferred_operand->SyntaxError("Unknown identifier");
      } else if (entry->instruction != NULL) {
        instruction->operand = entry->instruction->address;
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

    // Get opcode ************************************************
    u8 opcode = 0;
    AddressingMode mode = instruction->mode;
    InstructionType type = instruction->type;
    bool error = false;

    opcode = gInstructionToOpcode[type][mode];

    if (opcode == 0x00 && type != I_BRK) {
      bool found = false;
      for (int o = 0; o <= 256; o++) {
        InstructionTypeAndMode tm = gOpcodeToInstruction[o];
        if (tm.type == type && tm.mode == mode) {
          opcode = (u8)o;
          found = true;
          gInstructionToOpcode[type][mode] = opcode;
          break;
        }
      }
      if (!found) {
        error = true;
      }
    }

    int bytes = gBytesForAddressingMode[mode];

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
