
struct rect
{
    int X;
    int Y;
    int dX;
    int dY;
    int Width;
    u32 Color;
};

global rect GlobalRect;


internal void
DEBUGDrawRectangle(int X, int Y, int Width, int Height, u32 Color)
{
    int BytesPerPixel = 4;
    int Pitch = SCREEN_WIDTH * BytesPerPixel;
    u8 *Row = (u8 *)GlobalVideoMemory + Pitch * Y + X * BytesPerPixel;

    for (int pY = Y; pY < Y + Height; pY++)
    {
        u32 *Pixel = (u32 *) Row;
        for (int pX = X; pX < X + Width; pX++)
        {
            *Pixel++ = Color;
        }
        Row += Pitch;
    }
}






