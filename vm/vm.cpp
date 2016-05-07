// 64 Kb
#define MEMORY_SIZE 65536
#define SCREEN_WIDTH 280
#define SCREEN_HEIGHT 192
#define PIXEL_SIZE 2

// Must be filled with real client size
global int kWindowWidth;
global int kWindowHeight;


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
  int Pitch = SCREEN_WIDTH;
  u8 *Row = (u8 *)gVideoMemory + Pitch * Y + X;

  for (int pY = Y; pY < Y + Height; pY++) {
    u8 *Pixel = Row;
    for (int pX = X; pX < X + Width; pX++) {
      *Pixel++ = Color;
    }
    Row += Pitch;
  }
}

internal void MachineTick() {
  if (GlobalRect.Width == 0) {
    // Init
    GlobalRect.X = GlobalRect.Y = 50;
    GlobalRect.dX = GlobalRect.dY = 1;
    GlobalRect.Width = 5;
    GlobalRect.Color = 2;
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
  if (GlobalRect.X > SCREEN_WIDTH - GlobalRect.Width) {
    GlobalRect.X = SCREEN_WIDTH - GlobalRect.Width;
    GlobalRect.dX = -GlobalRect.dX;
  }
  if (GlobalRect.Y > SCREEN_HEIGHT - GlobalRect.Width) {
    GlobalRect.Y = SCREEN_HEIGHT - GlobalRect.Width;
    GlobalRect.dY = -GlobalRect.dY;
  }

  // Draw
  DEBUGDrawRectangle(GlobalRect.X, GlobalRect.Y, GlobalRect.Width,
                     GlobalRect.Width, GlobalRect.Color);
}
