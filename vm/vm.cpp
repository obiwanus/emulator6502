// ================= Virtual 6502-based machine ==================

global int const kMachineMemorySize = 65536;  // 64 Kb

global int const kWindowWidth = 280;
global int const kWindowHeight = 192;

global void *gMachineMemory;
global u8 *gVideoMemory;

#define SCREEN_ZOOM 4

struct cpu {
  u8 A;
  u8 X;
  u8 Y;
  u8 SP;
  u8 status;
  u16 PC;
};

global cpu CPU;


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

internal void CopyPixels(void *BitmapMemory, u8 *VideoMemory) {
  // Copy data from the machine's video memory to our "display"
  for (int y = 0; y < kWindowHeight; y++) {
    for (int x = 0; x < kWindowWidth; x++) {
      u8 *SrcPixel = VideoMemory + kWindowWidth * y + x;
      u32 *DestPixel = (u32 *)BitmapMemory + (kWindowWidth * y + x);
      *DestPixel = GetColor(*SrcPixel);
    }
  }
}


// TODO: delete ==================================================
struct rect {
  int X;
  int Y;
  int dX;
  int dY;
  int Width;
  u8 Color;
};

global rect GlobalRect;

internal void DEBUGDrawRectangle(int X, int Y, int Width, int Height,
                                 u8 Color) {
  int Pitch = kWindowWidth;
  u8 *Row = (u8 *)gVideoMemory + Pitch * Y + X;

  for (int pY = Y; pY < Y + Height; pY++) {
    u8 *Pixel = Row;
    for (int pX = X; pX < X + Width; pX++) {
      *Pixel++ = Color;
    }
    Row += Pitch;
  }
}

global int gCounter = 0;

internal void MachineTick() {
  if (GlobalRect.Width == 0) {
    // Init
    GlobalRect.X = GlobalRect.Y = 50;
    GlobalRect.dX = GlobalRect.dY = 1;
    GlobalRect.Width = 2;
    GlobalRect.Color = 1;
  }

  if (gCounter++ % 10 == 0) {
    GlobalRect.Color = ++GlobalRect.Color % 0xF;
  }

  // Erase
  // DEBUGDrawRectangle(GlobalRect.X, GlobalRect.Y, GlobalRect.Width,
  //                    GlobalRect.Width, 0);

  GlobalRect.X += GlobalRect.dX;
  GlobalRect.Y += GlobalRect.dY;

  if (GlobalRect.X < 0) {
    GlobalRect.X = 0;
    GlobalRect.dX = -GlobalRect.dX;
  }
  if (GlobalRect.Y < 0) {
    GlobalRect.Y = 0;
    GlobalRect.dY = -GlobalRect.dY;
  }
  if (GlobalRect.X > kWindowWidth - GlobalRect.Width) {
    GlobalRect.X = kWindowWidth - GlobalRect.Width;
    GlobalRect.dX = -GlobalRect.dX;
  }
  if (GlobalRect.Y > kWindowHeight - GlobalRect.Width) {
    GlobalRect.Y = kWindowHeight - GlobalRect.Width;
    GlobalRect.dY = -GlobalRect.dY;
  }

  // Draw
  DEBUGDrawRectangle(GlobalRect.X, GlobalRect.Y, GlobalRect.Width,
                     GlobalRect.Width, GlobalRect.Color);
}
