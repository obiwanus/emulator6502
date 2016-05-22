// ================= Virtual 6502-based machine ==================

global int const kMachineMemorySize = 65536;  // 64 Kb

global int const kWindowWidth = 280;
global int const kWindowHeight = 192;

global u16 const kPC_start = 0xD400;

global void *gMachineMemory;
global u8 *gVideoMemory;

#include "utils.cpp"
#include "asm.cpp"

#define SCREEN_ZOOM 4

#define FLAG_C 0x01
#define FLAG_Z 0x02
#define FLAG_I 0x04
#define FLAG_D 0x08
#define FLAG_B 0x10
#define FLAG_V 0x40
#define FLAG_S 0x80

struct CPU {
  u8 A;
  u8 X;
  u8 Y;
  u8 SP;
  u8 status;
  u16 PC;

  u8 *memory;

  CPU();
  void Tick();

  inline bool GetC();
  inline bool GetZ();
  inline bool GetI();
  inline bool GetD();
  inline bool GetB();
  inline bool GetV();
  inline bool GetS();

  inline void SetC(int);
  inline void SetZ(int);
  inline void SetI(int);
  inline void SetD(int);
  inline void SetB(int);
  inline void SetV(int);
  inline void SetS(int);
};

inline bool CPU::GetC() {
  return (this->status & FLAG_C) > 0;
}

inline bool CPU::GetZ() {
  return (this->status & FLAG_Z) > 0;
}

inline bool CPU::GetI() {
  return (this->status & FLAG_I) > 0;
}

inline bool CPU::GetD() {
  return (this->status & FLAG_D) > 0;
}

inline bool CPU::GetB() {
  return (this->status & FLAG_B) > 0;
}

inline bool CPU::GetV() {
  return (this->status & FLAG_V) > 0;
}

inline bool CPU::GetS() {
  return (this->status & FLAG_S) > 0;
}

inline void CPU::SetC(int value) {
  if (value) {
    this->status |= FLAG_C;
  } else {
    this->status &= ~FLAG_C;
  }
}

inline void CPU::SetZ(int value) {
  if (value) {
    this->status |= FLAG_Z;
  } else {
    this->status &= ~FLAG_Z;
  }
}

inline void CPU::SetI(int value) {
  if (value) {
    this->status |= FLAG_I;
  } else {
    this->status &= ~FLAG_I;
  }
}

inline void CPU::SetD(int value) {
  if (value) {
    this->status |= FLAG_D;
  } else {
    this->status &= ~FLAG_D;
  }
}

inline void CPU::SetB(int value) {
  if (value) {
    this->status |= FLAG_B;
  } else {
    this->status &= ~FLAG_B;
  }
}

inline void CPU::SetV(int value) {
  if (value) {
    this->status |= FLAG_V;
  } else {
    this->status &= ~FLAG_V;
  }
}

inline void CPU::SetS(int value) {
  if (value) {
    this->status |= FLAG_S;
  } else {
    this->status &= ~FLAG_S;
  }
}

inline u32 GetColor(u8 code) {
  u32 value = 0;
  switch (code) {
    case 0: {
      value = 0x000000;  // black
    } break;
    case 1: {
      value = 0xFF00FF;  // magenta
    } break;
    case 2: {
      value = 0x00008B;  // dark blue
    } break;
    case 3: {
      value = 0x800080;  // purple
    } break;
    case 4: {
      value = 0x006400;  // dark green
    } break;
    case 5: {
      value = 0xA8A8A8;  // dark grey
    } break;
    case 6: {
      value = 0x0000CD;  // medium blue
    } break;
    case 7: {
      value = 0xADD8E6;  // light blue
    } break;
    case 8: {
      value = 0x8B4513;  // brown
    } break;
    case 9: {
      value = 0xFFA500;  // orange
    } break;
    case 10: {
      value = 0xD3D3D3;  // light grey
    } break;
    case 11: {
      value = 0xFF69B4;  // pink
    } break;
    case 12: {
      value = 0x90EE90;  // light green
    } break;
    case 13: {
      value = 0xFFFF00;  // yellow
    } break;
    case 14: {
      value = 0x00FFFF;  // cyan
    } break;
    case 15: {
      value = 0xFFFFFF;  // white
    } break;
    default: {
      value = 0x00FF00;  // very bright green
    } break;
  }
  return value;
}

static void CopyPixels(void *BitmapMemory, u8 *VideoMemory) {
  // Copy data from the machine's video memory to our "display"
  for (int y = 0; y < kWindowHeight; y++) {
    for (int x = 0; x < kWindowWidth; x++) {
      u8 *SrcPixel = VideoMemory + kWindowWidth * y + x;
      u32 *DestPixel = (u32 *)BitmapMemory + (kWindowWidth * y + x);
      *DestPixel = GetColor(*SrcPixel);
    }
  }
}

CPU::CPU() {
  this->A = 0;
  this->X = 0;
  this->Y = 0;
  this->SP = 0;
  this->status = 0;
  this->PC = kPC_start;
  this->memory = (u8 *)gMachineMemory;
}

void CPU::Tick() {
  AddressingMode mode = AM_Unknown;
  InstructionType type = I_Unknown;

  u8 opcode = this->memory[this->PC];

  if (mode == AM_Unknown) {
    type = I_ADC;
    switch (opcode) {
      case 0x69: {
        mode = AM_Immediate;
      } break;
      case 0x65: {
        mode = AM_Zeropage;
      } break;
      case 0x75: {
        mode = AM_Zeropage_X;
      } break;
      case 0x6D: {
        mode = AM_Absolute;
      } break;
      case 0x7D: {
        mode = AM_Absolute_X;
      } break;
      case 0x79: {
        mode = AM_Absolute_Y;
      } break;
      case 0x61: {
        mode = AM_Indirect_X;
      } break;
      case 0x71: {
        mode = AM_Indirect_Y;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_AND;
    switch (opcode) {
      case 0x29: {
        mode = AM_Immediate;
      } break;
      case 0x25: {
        mode = AM_Zeropage;
      } break;
      case 0x35: {
        mode = AM_Zeropage_X;
      } break;
      case 0x2D: {
        mode = AM_Absolute;
      } break;
      case 0x3D: {
        mode = AM_Absolute_X;
      } break;
      case 0x39: {
        mode = AM_Absolute_Y;
      } break;
      case 0x21: {
        mode = AM_Indirect_X;
      } break;
      case 0x31: {
        mode = AM_Indirect_Y;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_ASL;
    switch (opcode) {
      case 0x0A: {
        mode = AM_Accumulator;
      } break;
      case 0x06: {
        mode = AM_Zeropage;
      } break;
      case 0x16: {
        mode = AM_Zeropage_X;
      } break;
      case 0x0E: {
        mode = AM_Absolute;
      } break;
      case 0x1E: {
        mode = AM_Absolute_X;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_BIT;
    switch (opcode) {
      case 0x2C: {
        mode = AM_Absolute;
      } break;
      case 0x24: {
        mode = AM_Zeropage;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_CMP;
    switch (opcode) {
      case 0xC9: {
        mode = AM_Immediate;
      } break;
      case 0xC5: {
        mode = AM_Zeropage;
      } break;
      case 0xD5: {
        mode = AM_Zeropage_X;
      } break;
      case 0xCD: {
        mode = AM_Absolute;
      } break;
      case 0xDD: {
        mode = AM_Absolute_X;
      } break;
      case 0xD9: {
        mode = AM_Absolute_Y;
      } break;
      case 0xC1: {
        mode = AM_Indirect_X;
      } break;
      case 0xD1: {
        mode = AM_Indirect_Y;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_CPX;
    switch (opcode) {
      case 0xE0: {
        mode = AM_Immediate;
      } break;
      case 0xE4: {
        mode = AM_Zeropage;
      } break;
      case 0xEC: {
        mode = AM_Absolute;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_CPY;
    switch (opcode) {
      case 0xC0: {
        mode = AM_Immediate;
      } break;
      case 0xC4: {
        mode = AM_Zeropage;
      } break;
      case 0xCC: {
        mode = AM_Absolute;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_DEC;
    switch (opcode) {
      case 0xC6: {
        mode = AM_Zeropage;
      } break;
      case 0xD6: {
        mode = AM_Zeropage_X;
      } break;
      case 0xCE: {
        mode = AM_Absolute;
      } break;
      case 0xDE: {
        mode = AM_Absolute_X;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_EOR;
    switch (opcode) {
      case 0x49: {
        mode = AM_Immediate;
      } break;
      case 0x45: {
        mode = AM_Zeropage;
      } break;
      case 0x55: {
        mode = AM_Zeropage_X;
      } break;
      case 0x4D: {
        mode = AM_Absolute;
      } break;
      case 0x5D: {
        mode = AM_Absolute_X;
      } break;
      case 0x59: {
        mode = AM_Absolute_Y;
      } break;
      case 0x41: {
        mode = AM_Indirect_X;
      } break;
      case 0x51: {
        mode = AM_Indirect_Y;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_INC;
    switch (opcode) {
      case 0xE6: {
        mode = AM_Zeropage;
      } break;
      case 0xF6: {
        mode = AM_Zeropage_X;
      } break;
      case 0xEE: {
        mode = AM_Absolute;
      } break;
      case 0xFE: {
        mode = AM_Absolute_X;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_JMP;
    switch (opcode) {
      case 0x4C: {
        mode = AM_Absolute;
      } break;
      case 0x6C: {
        mode = AM_Indirect;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_JSR;
    switch (opcode) {
      case 0x20: {
        mode = AM_Absolute;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_LDA;
    switch (opcode) {
      case 0xA9: {
        mode = AM_Immediate;
      } break;
      case 0xA5: {
        mode = AM_Zeropage;
      } break;
      case 0xB5: {
        mode = AM_Zeropage_X;
      } break;
      case 0xAD: {
        mode = AM_Absolute;
      } break;
      case 0xBD: {
        mode = AM_Absolute_X;
      } break;
      case 0xB9: {
        mode = AM_Absolute_Y;
      } break;
      case 0xA1: {
        mode = AM_Indirect_X;
      } break;
      case 0xB1: {
        mode = AM_Indirect_Y;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_LDX;
    switch (opcode) {
      case 0xA2: {
        mode = AM_Immediate;
      } break;
      case 0xA6: {
        mode = AM_Zeropage;
      } break;
      case 0xB6: {
        mode = AM_Zeropage_Y;
      } break;
      case 0xAE: {
        mode = AM_Absolute;
      } break;
      case 0xBE: {
        mode = AM_Absolute_Y;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_LDY;
    switch (opcode) {
      case 0xA0: {
        mode = AM_Immediate;
      } break;
      case 0xA4: {
        mode = AM_Zeropage;
      } break;
      case 0xB4: {
        mode = AM_Zeropage_X;
      } break;
      case 0xAC: {
        mode = AM_Absolute;
      } break;
      case 0xBC: {
        mode = AM_Absolute_X;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_LSR;
    switch (opcode) {
      case 0x4A: {
        mode = AM_Accumulator;
      } break;
      case 0x46: {
        mode = AM_Zeropage;
      } break;
      case 0x56: {
        mode = AM_Zeropage_X;
      } break;
      case 0x4E: {
        mode = AM_Absolute;
      } break;
      case 0x5E: {
        mode = AM_Absolute_X;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_ORA;
    switch (opcode) {
      case 0x09: {
        mode = AM_Immediate;
      } break;
      case 0x05: {
        mode = AM_Zeropage;
      } break;
      case 0x15: {
        mode = AM_Zeropage_X;
      } break;
      case 0x0D: {
        mode = AM_Absolute;
      } break;
      case 0x1D: {
        mode = AM_Absolute_X;
      } break;
      case 0x19: {
        mode = AM_Absolute_Y;
      } break;
      case 0x01: {
        mode = AM_Indirect_X;
      } break;
      case 0x11: {
        mode = AM_Indirect_Y;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_ROL;
    switch (opcode) {
      case 0x2A: {
        mode = AM_Accumulator;
      } break;
      case 0x26: {
        mode = AM_Zeropage;
      } break;
      case 0x36: {
        mode = AM_Zeropage_X;
      } break;
      case 0x2E: {
        mode = AM_Absolute;
      } break;
      case 0x3E: {
        mode = AM_Absolute_X;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_ROR;
    switch (opcode) {
      case 0x6A: {
        mode = AM_Accumulator;
      } break;
      case 0x66: {
        mode = AM_Zeropage;
      } break;
      case 0x76: {
        mode = AM_Zeropage_X;
      } break;
      case 0x6E: {
        mode = AM_Absolute;
      } break;
      case 0x7E: {
        mode = AM_Absolute_X;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_SBC;
    switch (opcode) {
      case 0xE9: {
        mode = AM_Immediate;
      } break;
      case 0xE5: {
        mode = AM_Zeropage;
      } break;
      case 0xF5: {
        mode = AM_Zeropage_X;
      } break;
      case 0xED: {
        mode = AM_Absolute;
      } break;
      case 0xFD: {
        mode = AM_Absolute_X;
      } break;
      case 0xF9: {
        mode = AM_Absolute_Y;
      } break;
      case 0xE1: {
        mode = AM_Indirect_X;
      } break;
      case 0xF1: {
        mode = AM_Indirect_Y;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_STA;
    switch (opcode) {
      case 0x85: {
        mode = AM_Zeropage;
      } break;
      case 0x95: {
        mode = AM_Zeropage_X;
      } break;
      case 0x8D: {
        mode = AM_Absolute;
      } break;
      case 0x9D: {
        mode = AM_Absolute_X;
      } break;
      case 0x99: {
        mode = AM_Absolute_Y;
      } break;
      case 0x81: {
        mode = AM_Indirect_X;
      } break;
      case 0x91: {
        mode = AM_Indirect_Y;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_STX;
    switch (opcode) {
      case 0x86: {
        mode = AM_Zeropage;
      } break;
      case 0x96: {
        mode = AM_Zeropage_Y;
      } break;
      case 0x8E: {
        mode = AM_Absolute;
      } break;
    }
  }
  if (mode == AM_Unknown) {
    type = I_STY;
    switch (opcode) {
      case 0x84: {
        mode = AM_Zeropage;
      } break;
      case 0x94: {
        mode = AM_Zeropage_X;
      } break;
      case 0x8C: {
        mode = AM_Absolute;
      } break;
    }
  }

  if (mode == AM_Unknown) {
    mode = AM_Implied;
    switch (opcode) {
      case 0x00: {
        type = I_BRK;
      } break;
      case 0x18: {
        type = I_CLC;
      } break;
      case 0xD8: {
        type = I_CLD;
      } break;
      case 0x58: {
        type = I_CLI;
      } break;
      case 0xB8: {
        type = I_CLV;
      } break;
      case 0xCA: {
        type = I_DEX;
      } break;
      case 0x88: {
        type = I_DEY;
      } break;
      case 0xE8: {
        type = I_INX;
      } break;
      case 0xC8: {
        type = I_INY;
      } break;
      case 0xEA: {
        type = I_NOP;
      } break;
      case 0x48: {
        type = I_PHA;
      } break;
      case 0x08: {
        type = I_PHP;
      } break;
      case 0x68: {
        type = I_PLA;
      } break;
      case 0x28: {
        type = I_PLP;
      } break;
      case 0x40: {
        type = I_RTI;
      } break;
      case 0x60: {
        type = I_RTS;
      } break;
      case 0x38: {
        type = I_SEC;
      } break;
      case 0xF8: {
        type = I_SED;
      } break;
      case 0x78: {
        type = I_SEI;
      } break;
      case 0xAA: {
        type = I_TAX;
      } break;
      case 0xA8: {
        type = I_TAY;
      } break;
      case 0xBA: {
        type = I_TSX;
      } break;
      case 0x8A: {
        type = I_TXA;
      } break;
      case 0x9A: {
        type = I_TXS;
      } break;
      case 0x98: {
        type = I_TYA;
      } break;

      default: { mode = AM_Unknown; } break;
    }
  }

  if (mode == AM_Unknown) {
    mode = AM_Relative;
    switch (opcode) {
      case 0x90: {
        type = I_BCC;
      } break;
      case 0xB0: {
        type = I_BCS;
      } break;
      case 0xF0: {
        type = I_BEQ;
      } break;
      case 0x30: {
        type = I_BMI;
      } break;
      case 0xD0: {
        type = I_BNE;
      } break;
      case 0x10: {
        type = I_BPL;
      } break;
      case 0x50: {
        type = I_BVC;
      } break;
      case 0x70: {
        type = I_BVS;
      } break;

      default: { mode = AM_Unknown; } break;
    }
  }

  // Default
  if (mode == AM_Unknown) {
    type = I_NOP;
    mode = AM_Implied;
    print("WARNING: Unknown instruction treated as NOP, opcode %#02x\n",
          opcode);
  }

  int bytes = BytesForAddressingMode(mode);
  if (!bytes) {
    print("Panic (incorrect instruction length)!\n");
    exit(1);
  }

  // Fetch operand
  int operand = 0;
  if (bytes == 2) {
    operand = (int)this->memory[this->PC + 1];
  } else if (bytes == 3) {
    operand =
        (int)(this->memory[this->PC + 2] << 8 | this->memory[this->PC + 1]);
  }

  // Moving the PC now, as it may change later
  this->PC += (u16)bytes;

  // Get the data according to the addressing mode
  u8 data = 0;
  u8 *data_pointer = NULL;
  u16 address = 0;
  switch (mode) {
    case AM_Immediate: {
      data = (u8)operand;
    } break;
    case AM_Relative:
    case AM_Absolute:
    case AM_Zeropage: {
      data_pointer = this->memory + operand;
    } break;
    case AM_Absolute_X:
    case AM_Zeropage_X: {
      data_pointer = this->memory + operand + this->X;
    } break;
    case AM_Absolute_Y:
    case AM_Zeropage_Y: {
      data_pointer = this->memory + operand + this->Y;
    } break;
    case AM_Indirect: {
      address = (u16)(this->memory[operand + 1] << 8 | this->memory[operand]);
      data_pointer = this->memory + address;
    } break;
    case AM_Indirect_X: {
      address = (u16)(this->memory[operand + this->X + 1] << 8 |
                      this->memory[operand + this->X]);
      data_pointer = this->memory + address;
    } break;
    case AM_Indirect_Y: {
      address = (u16)(this->memory[operand + 1] << 8 | this->memory[operand]);
      data_pointer = this->memory + address + this->Y;
    } break;
    case AM_Implied: {
    } break;
    case AM_Accumulator: {
      data = this->A;
    } break;
    default: {
      print("Panic (incorrect mode)!\n");
      exit(1);
    }
  }

  if (data_pointer != NULL) {
    data = *data_pointer;
  }

  // Execute instruction
  switch (type) {
    case I_LDA: {
      this->A = data;
    } break;
    case I_STA: {
      *data_pointer = this->A;
    } break;
    case I_JMP: {
      this->PC = data;
    } break;
    case I_INX: {
      this->X++;
    } break;
    case I_NOP: {
      // you lazy bastard!
    } break;
    default: {
      print("Panic! Unknown instruction, opcode %#02x\n", opcode);
      exit(1);
    }
  }
}
