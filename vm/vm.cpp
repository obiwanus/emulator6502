// ================= Virtual 6502-based machine ==================

global int const kMachineMemorySize = 65536;  // 64 Kb

global int const kWindowWidth = 280;
global int const kWindowHeight = 192;


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
