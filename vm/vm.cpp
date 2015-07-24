
// TODO: delete
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


typedef union
{
    u64 all64;
    struct
    {
        u32 high32;
        u32 low32;
    };
} register64;


struct cpu
{
    u64 sp;
    u64 bp;

    u64 cs;
    u64 ds;
    u64 ss;
    u64 es;

    u64 ip;
    u64 flags;

    // Multi purpose
    register64 a;
    register64 b;
    register64 c;
    register64 d;
    register64 e;
    register64 f;

};


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


internal void
MachineTick()
{
    if (GlobalRect.Width == 0)
    {
        // Init
        GlobalRect.X = GlobalRect.Y = 50;
        GlobalRect.dX = GlobalRect.dY = 1;
        GlobalRect.Width = 10;
        GlobalRect.Color = 0x00aacc00;
    }

    DEBUGDrawRectangle(GlobalRect.X, GlobalRect.Y, GlobalRect.Width,
                       GlobalRect.Width, 0x00000000);

    GlobalRect.X += GlobalRect.dX;
    GlobalRect.Y += GlobalRect.dY;

    if (GlobalRect.X < 0)
    {
        GlobalRect.X = 0;
        GlobalRect.dX = -GlobalRect.dX;
    }
    if (GlobalRect.Y < 0)
    {
        GlobalRect.Y = 0;
        GlobalRect.dY = -GlobalRect.dY;
    }
    if (GlobalRect.X > SCREEN_WIDTH - GlobalRect.Width)
    {
        GlobalRect.X = SCREEN_WIDTH - GlobalRect.Width;
        GlobalRect.dX = -GlobalRect.dX;
    }
    if (GlobalRect.Y > SCREEN_HEIGHT - GlobalRect.Width)
    {
        GlobalRect.Y = SCREEN_HEIGHT - GlobalRect.Width;
        GlobalRect.dY = -GlobalRect.dY;
    }

    DEBUGDrawRectangle(GlobalRect.X, GlobalRect.Y, GlobalRect.Width,
                       GlobalRect.Width, GlobalRect.Color);

}



