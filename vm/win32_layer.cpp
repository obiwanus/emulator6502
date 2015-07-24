#include "base.h"

#define MEMORY_SIZE (1024 * 1024 * 1024)
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

global void *GlobalMachineMemory;
global void *GlobalVideoMemory;
global volatile bool32 GlobalRunning;

#include "vm.cpp"

#include <windows.h>
#include <intrin.h>

global BITMAPINFO GlobalBitmapInfo;


internal void
Win32UpdateWindow(HDC hdc)
{
    if (!GlobalVideoMemory)
        return;

    StretchDIBits(
        hdc,
        0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,  // dest
        0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,  // src
        GlobalVideoMemory,
        &GlobalBitmapInfo,
        DIB_RGB_COLORS, SRCCOPY);
}


DWORD WINAPI
MachineTick(LPVOID lpParam)
{
    if (GlobalRect.Width == 0)
    {
        // Init
        GlobalRect.X = GlobalRect.Y = 50;
        GlobalRect.dX = GlobalRect.dY = 1;
        GlobalRect.Width = 10;
        GlobalRect.Color = 0x00aacc00;
    }

    while (GlobalRunning)
    {
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

        Sleep(30);
    }

    return 0;
}


LRESULT CALLBACK
Win32WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT Result = 0;

    switch(uMsg)
    {
        case WM_CLOSE:
        {
            GlobalRunning = false;
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint = {};
            HDC hdc = BeginPaint(hwnd, &Paint);
            Win32UpdateWindow(hdc);
            EndPaint(hwnd, &Paint);
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert(!"Keyboard input came in through a non-dispatch message!");
        } break;

        default:
        {
            Result = DefWindowProc(hwnd, uMsg, wParam, lParam);
        } break;
    }

    return Result;
}


int CALLBACK
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
    WNDCLASS WindowClass = {};
    WindowClass.style = CS_OWNDC|CS_VREDRAW|CS_HREDRAW;
    WindowClass.lpfnWndProc = Win32WindowProc;
    WindowClass.hInstance = hInstance;
    WindowClass.lpszClassName = "VMWindowClass";

    if (RegisterClass(&WindowClass))
    {
        HWND Window = CreateWindow(
            WindowClass.lpszClassName,
            0,
            WS_OVERLAPPEDWINDOW^WS_THICKFRAME|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            0,
            0,
            hInstance,
            0);

        if (Window)
        {
            // We're not going to release it as we use CS_OWNDC
            HDC hdc = GetDC(Window);

            GlobalRunning = true;

            // Init memory
            GlobalMachineMemory = VirtualAlloc(0, MEMORY_SIZE, MEM_COMMIT, PAGE_READWRITE);
            GlobalVideoMemory = GlobalMachineMemory;

            // Init bitmap
            GlobalBitmapInfo.bmiHeader.biWidth = SCREEN_WIDTH;
            GlobalBitmapInfo.bmiHeader.biHeight = SCREEN_HEIGHT;
            GlobalBitmapInfo.bmiHeader.biSize = sizeof(GlobalBitmapInfo.bmiHeader);
            GlobalBitmapInfo.bmiHeader.biPlanes = 1;
            GlobalBitmapInfo.bmiHeader.biBitCount = 32;
            GlobalBitmapInfo.bmiHeader.biCompression = BI_RGB;

            // Run the machine
            HANDLE MainMachineThread =  CreateThread(0, 0, MachineTick, 0, 0, 0);

            // Event loop
            while (GlobalRunning)
            {
                // Process messages
                MSG Message;
                while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    // Get keyboard messages
                    switch (Message.message)
                    {
                        case WM_QUIT:
                        {
                            GlobalRunning = false;
                        } break;

                        case WM_SYSKEYDOWN:
                        case WM_SYSKEYUP:
                        case WM_KEYDOWN:
                        case WM_KEYUP:
                        {
                            u32 VKCode = (u32)Message.wParam;
                            bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                            bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);

                            bool32 AltKeyWasDown = (Message.lParam & (1 << 29));
                            if((VKCode == VK_F4) && AltKeyWasDown)
                            {
                                GlobalRunning = false;
                            }
                        } break;

                        default:
                        {
                            TranslateMessage(&Message);
                            DispatchMessageA(&Message);
                        } break;
                    }
                }

                // TODO: sleep on vblank
                Win32UpdateWindow(hdc);
                Sleep(30);
            }
        }
    }
    else
    {
        // TODO: logging
        OutputDebugStringA("Couldn't register window class");
    }

    return 0;
}