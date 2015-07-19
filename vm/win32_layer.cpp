#include "base.h"

#define MEMORY_SIZE (1024 * 1024 * 1024)
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

global void *GlobalMachineMemory;
global void *GlobalVideoMemory;

#include "vm.cpp"

#include <windows.h>
#include <intrin.h>

global BITMAPINFO GlobalBitmapInfo;
global bool32 GlobalRunning;


internal void
Win32UpdateWindow(HDC hdc, void *VideoMemory)
{
    if (!VideoMemory)
        return;

    StretchDIBits(
        hdc,
        0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,  // dest
        0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,  // src
        VideoMemory,
        &GlobalBitmapInfo,
        DIB_RGB_COLORS, SRCCOPY);
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
            Win32UpdateWindow(hdc, GlobalVideoMemory);
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


internal void
Win32ProcessPendingMessages()
{
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
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            0,
            0,
            hInstance,
            0);

        // We're not going to release it as we use CS_OWNDC
        HDC hdc = GetDC(Window);



        if (Window)
        {
            GlobalRunning = true;

            // Init memory
            GlobalMachineMemory = VirtualAlloc(0, MEMORY_SIZE, MEM_COMMIT, PAGE_READWRITE);
            GlobalVideoMemory = GlobalMachineMemory;

            // Init bitmap
            GlobalBitmapInfo.bmiHeader.biSize = sizeof(GlobalBitmapInfo.bmiHeader);
            GlobalBitmapInfo.bmiHeader.biPlanes = 1;
            GlobalBitmapInfo.bmiHeader.biBitCount = 32;
            GlobalBitmapInfo.bmiHeader.biCompression = BI_RGB;

            // Main loop
            while (GlobalRunning)
            {
                // Tick

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