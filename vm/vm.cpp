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
};

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
    mode = AM_Implied;
    switch (opcode) {
      case 0xE8: {
        type = I_INX;
      } break;

      default: {
        mode = AM_Unknown;
      } break;
    }
  }

  // Default
  if (mode == AM_Unknown) {
    type = I_NOP;
    mode = AM_Implied;
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
      print("Panic! (unknown instruction)\n");
      exit(1);
    }
  }
}
