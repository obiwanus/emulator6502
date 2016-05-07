// 64 Kb
#define MEMORY_SIZE 65536
#define SCREEN_WIDTH 280
#define SCREEN_HEIGHT 192
#define PIXEL_SIZE 2

// Must be filled with real client size
global int kWindowWidth;
global int kWindowHeight;



internal void MachineTick() {
  u8 *Pixel = gVideoMemory + 30 + 20 * SCREEN_WIDTH;
  *Pixel++ = 1;
  *Pixel++ = 2;
  *Pixel = 3;
  *(Pixel + SCREEN_WIDTH) = 1;
  *(Pixel + SCREEN_WIDTH * 2) = 2;
  *(Pixel + SCREEN_WIDTH * 3) = 3;
}




// TODO: delete ==================================================
// struct rect {
//   int X;
//   int Y;
//   int dX;
//   int dY;
//   int Width;
//   u32 Color;
// };

// global rect GlobalRect;

// internal void DEBUGDrawRectangle(int X, int Y, int Width, int Height,
//                                  u32 Color) {
//   int BytesPerPixel = 4;
//   int Pitch = kWindowWidth * BytesPerPixel;
//   u8 *Row = (u8 *)gVideoMemory + Pitch * Y + X * BytesPerPixel;

//   for (int pY = Y; pY < Y + Height; pY++) {
//     u32 *Pixel = (u32 *)Row;
//     for (int pX = X; pX < X + Width; pX++) {
//       *Pixel++ = Color;
//     }
//     Row += Pitch;
//   }
// }

// internal void MachineTick() {
//   if (GlobalRect.Width == 0) {
//     // Init
//     GlobalRect.X = GlobalRect.Y = 50;
//     GlobalRect.dX = GlobalRect.dY = 1;
//     GlobalRect.Width = 10;
//     GlobalRect.Color = 0x00aacc00;
//   }

//   DEBUGDrawRectangle(GlobalRect.X, GlobalRect.Y, GlobalRect.Width,
//                      GlobalRect.Width, 0x00000000);

//   GlobalRect.X += GlobalRect.dX;
//   GlobalRect.Y += GlobalRect.dY;

//   if (GlobalRect.X < 0) {
//     GlobalRect.X = 0;
//     GlobalRect.dX = -GlobalRect.dX;
//   }
//   if (GlobalRect.Y < 0) {
//     GlobalRect.Y = 0;
//     GlobalRect.dY = -GlobalRect.dY;
//   }
//   if (GlobalRect.X > kWindowWidth - GlobalRect.Width) {
//     GlobalRect.X = kWindowWidth - GlobalRect.Width;
//     GlobalRect.dX = -GlobalRect.dX;
//   }
//   if (GlobalRect.Y > kWindowHeight - GlobalRect.Width) {
//     GlobalRect.Y = kWindowHeight - GlobalRect.Width;
//     GlobalRect.dY = -GlobalRect.dY;
//   }

//   DEBUGDrawRectangle(GlobalRect.X, GlobalRect.Y, GlobalRect.Width,
//                      GlobalRect.Width, GlobalRect.Color);
// }
