// ================= Virtual 6502-based machine ==================

global int const kMachineMemorySize = 65536;  // 64 Kb

global int const kWindowWidth = 280;
global int const kWindowHeight = 192;

global u16 const kPC_start = 0xD400;
global int const kSP_start = 0x100;

global void *gMachineMemory;
global u8 *gVideoMemory;

#include "utils.cpp"
#include "asm.cpp"

#define SCREEN_ZOOM 4

inline u32 GetColor(u8 code) {
  if (code > 15) {
    return 0x00FF00;  // very bright green
  }
  u32 palette[16] = {
      0x000000,  // black
      0xFF00FF,  // magenta
      0x00008B,  // dark blue
      0x800080,  // purple
      0x006400,  // dark green
      0xA8A8A8,  // dark grey
      0x0000CD,  // medium blue
      0xADD8E6,  // light blue
      0x8B4513,  // brown
      0xFFA500,  // orange
      0xD3D3D3,  // light grey
      0xFF69B4,  // pink
      0x90EE90,  // light green
      0xFFFF00,  // yellow
      0x00FFFF,  // cyan
      0xFFFFFF,  // white
  };
  return palette[code];
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
  bool is_running;

  CPU();
  void Tick();

  inline bool GetC();
  inline bool GetZ();
  inline bool GetI();
  inline bool GetD();
  inline bool GetB();
  inline bool GetV();
  inline bool GetN();

  inline void SetC(int);
  inline void SetZ(int);
  inline void SetI(int);
  inline void SetD(int);
  inline void SetB(int);
  inline void SetV(int);
  inline void SetN(int);

  inline void SetNZFor(u8);
};

CPU::CPU() {
  this->A = 0;
  this->X = 0;
  this->Y = 0;
  this->SP = 0;
  this->status = 0;
  this->PC = kPC_start;
  this->memory = (u8 *)gMachineMemory;
  this->is_running = true;
}

inline bool CPU::GetC() { return (this->status & FLAG_C) > 0; }

inline bool CPU::GetZ() { return (this->status & FLAG_Z) > 0; }

inline bool CPU::GetI() { return (this->status & FLAG_I) > 0; }

inline bool CPU::GetD() { return (this->status & FLAG_D) > 0; }

inline bool CPU::GetB() { return (this->status & FLAG_B) > 0; }

inline bool CPU::GetV() { return (this->status & FLAG_V) > 0; }

inline bool CPU::GetN() { return (this->status & FLAG_S) > 0; }

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

inline void CPU::SetN(int value) {
  if (value) {
    this->status |= FLAG_S;
  } else {
    this->status &= ~FLAG_S;
  }
}

inline void CPU::SetNZFor(u8 value) {
  this->SetN(value >> 7 ? 1 : 0);
  this->SetZ(value == 0 ? 1 : 0);
}

void CPU::Tick() {
  u8 opcode = this->memory[this->PC];

  InstructionTypeAndMode instruction = gOpcodeToInstruction[opcode];

  // Default
  if (instruction.mode == AM_Unknown) {
    instruction.type = I_NOP;
    instruction.mode = AM_Implied;
    print("WARNING: Unknown instruction treated as NOP, opcode %#02x\n",
          opcode);
  }

  int bytes = gBytesForAddressingMode[instruction.mode];
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
  switch (instruction.mode) {
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
  switch (instruction.type) {
    case I_ADC: {
      u16 tmp = this->A + data + (u8) this->GetC();
      this->A = (u8)tmp;
      this->SetC(tmp > this->A ? 1 : 0);
    } break;
    case I_AND: {
      print("ERROR: instruction AND not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_ASL: {
      print("ERROR: instruction ASL not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_BIT: {
      print("ERROR: instruction BIT not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_CMP: {
      u8 tmp = this->A - data;
      this->SetNZFor(tmp);
      this->SetC(this->A >= data ? 1 : 0);
    } break;
    case I_CPX: {
      u8 tmp = this->X - data;
      this->SetNZFor(tmp);
      this->SetC(this->X >= data ? 1 : 0);
    } break;
    case I_CPY: {
      u8 tmp = this->Y - data;
      this->SetNZFor(tmp);
      this->SetC(this->Y >= data ? 1 : 0);
    } break;
    case I_DEC: {
      print("ERROR: instruction DEC not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_EOR: {
      print("ERROR: instruction EOR not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_INC: {
      *data_pointer = *data_pointer + 1;
      this->SetNZFor(*data_pointer);
    } break;
    case I_JMP: {
      this->PC = (u16)operand;
    } break;
    case I_JSR: {
      print("ERROR: instruction JSR not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_LDA: {
      this->A = data;
      this->SetNZFor(data);
    } break;
    case I_LDX: {
      this->X = data;
      this->SetNZFor(data);
    } break;
    case I_LDY: {
      this->Y = data;
      this->SetNZFor(data);
    } break;
    case I_LSR: {
      print("ERROR: instruction LSR not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_ORA: {
      print("ERROR: instruction ORA not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_ROL: {
      print("ERROR: instruction ROL not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_ROR: {
      print("ERROR: instruction ROR not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_SBC: {
      print("ERROR: instruction SBC not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_STA: {
      *data_pointer = this->A;
    } break;
    case I_STX: {
      print("ERROR: instruction STX not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_STY: {
      print("ERROR: instruction STY not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_BRK: {
      print("ERROR: instruction BRK not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_CLC: {
      print("ERROR: instruction CLC not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_CLD: {
      print("ERROR: instruction CLD not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_CLI: {
      print("ERROR: instruction CLI not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_CLV: {
      print("ERROR: instruction CLV not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_DEX: {
      print("ERROR: instruction DEX not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_DEY: {
      print("ERROR: instruction DEY not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_INX: {
      this->X++;
      this->SetNZFor(this->Y);
    } break;
    case I_INY: {
      this->Y++;
      this->SetNZFor(this->Y);
    } break;
    case I_NOP: {
      // You lazy bastard!
    } break;
    case I_PHA: {
      if (this->SP >= 0xFF) {
        print("Stack overflow\n");
        exit(1);
      }
      this->memory[kSP_start + this->SP] = this->A;
      this->SP++;
    } break;
    case I_PHP: {
      print("ERROR: instruction PHP not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_PLA: {
      if (this->SP == 0) {
        print("Stack underflow\n");
        exit(1);
      }
      this->SP--;
      this->A = this->memory[kSP_start + this->SP];
    } break;
    case I_PLP: {
      print("ERROR: instruction PLP not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_RTI: {
      print("ERROR: instruction RTI not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_RTS: {
      print("ERROR: instruction RTS not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_SEC: {
      print("ERROR: instruction SEC not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_SED: {
      print("ERROR: instruction SED not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_SEI: {
      print("ERROR: instruction SEI not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_TAX: {
      print("ERROR: instruction TAX not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_TAY: {
      print("ERROR: instruction TAY not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_TSX: {
      print("ERROR: instruction TSX not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_TXA: {
      print("ERROR: instruction TXA not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_TXS: {
      print("ERROR: instruction TXS not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_TYA: {
      print("ERROR: instruction TYA not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_BCC: {
      if (!this->GetC()) {
        this->PC = (u16)operand;
      }
    } break;
    case I_BCS: {
      if (this->GetC()) {
        this->PC = (u16)operand;
      }
    } break;
    case I_BEQ: {
      print("ERROR: instruction BEQ not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_BMI: {
      print("ERROR: instruction BMI not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_BNE: {
      if (!this->GetZ()) {
        this->PC = (u16)operand;
      }
    } break;
    case I_BPL: {
      print("ERROR: instruction BPL not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_BVC: {
      print("ERROR: instruction BVC not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_BVS: {
      print("ERROR: instruction BVS not implemented. Opcode %#02x\n", opcode);
      exit(1);
    } break;
    case I_END: {
      this->is_running = false;
    } break;

    default: {
      print("Panic! Unknown instruction, opcode %#02x\n", opcode);
      exit(1);
    }
  }
}
