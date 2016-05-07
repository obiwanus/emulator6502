#include "base.h"

global void *gMachineMemory;
global u8 *gVideoMemory;
global void *gWindowsBitmapMemory;
global volatile bool32 gRunning;

#include "vm.cpp"

#include <windows.h>
#include <intrin.h>

#define SCREEN_ZOOM 4

global BITMAPINFO GlobalBitmapInfo;

inline u32 Win32GetColor(u8 code) {
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

internal void Win32UpdateWindow(HDC hdc) {
  if (!gWindowsBitmapMemory) return;

  // Copy data from the machine's video memory to our "display"
  for (int y = 0; y < kWindowHeight; y++) {
    for (int x = 0; x < kWindowWidth; x++) {
      u8 *SrcPixel = gVideoMemory + kWindowWidth * y + x;
      u32 *DestPixel = (u32 *)gWindowsBitmapMemory + (kWindowWidth * y + x);
      *DestPixel = Win32GetColor(*SrcPixel);
    }
  }

  StretchDIBits(hdc, 0, 0, kWindowWidth * SCREEN_ZOOM, kWindowHeight * SCREEN_ZOOM,  // dest
                0, 0, kWindowWidth, kWindowHeight,       // src
                gWindowsBitmapMemory, &GlobalBitmapInfo, DIB_RGB_COLORS,
                SRCCOPY);
}

LRESULT CALLBACK
Win32WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  LRESULT Result = 0;

  switch (uMsg) {
    case WM_CLOSE: {
      gRunning = false;
    } break;

    case WM_PAINT: {
      PAINTSTRUCT Paint = {};
      HDC hdc = BeginPaint(hwnd, &Paint);
      Win32UpdateWindow(hdc);
      EndPaint(hwnd, &Paint);
    } break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP: {
      Assert(!"Keyboard input came in through a non-dispatch message!");
    } break;

    default: { Result = DefWindowProc(hwnd, uMsg, wParam, lParam); } break;
  }

  return Result;
}

DWORD WINAPI MachineThread(LPVOID lpParam) {
  while (gRunning) {
    MachineTick();
    Sleep(1);
  }
  return 0;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow) {
  WNDCLASS WindowClass = {};
  WindowClass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
  WindowClass.lpfnWndProc = Win32WindowProc;
  WindowClass.hInstance = hInstance;
  WindowClass.lpszClassName = "VMWindowClass";

  if (RegisterClass(&WindowClass)) {
    // Create window so that its client area is exactly kWindowWidth/Height
    DWORD WindowStyle = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME | WS_VISIBLE;
    RECT WindowRect = {};
    WindowRect.right = kWindowWidth * SCREEN_ZOOM;
    WindowRect.bottom = kWindowHeight * SCREEN_ZOOM;
    AdjustWindowRect(&WindowRect, WindowStyle, 0);
    int WindowWidth = WindowRect.right - WindowRect.left;
    int WindowHeight = WindowRect.bottom - WindowRect.top;
    HWND Window = CreateWindow(WindowClass.lpszClassName, 0, WindowStyle,
                               CW_USEDEFAULT, CW_USEDEFAULT, WindowWidth,
                               WindowHeight, 0, 0, hInstance, 0);

    if (Window) {
      // We're not going to release it as we use CS_OWNDC
      HDC hdc = GetDC(Window);

      gRunning = true;

      // Init memory
      gMachineMemory =
          VirtualAlloc(0, kMachineMemorySize, MEM_COMMIT, PAGE_READWRITE);
      gVideoMemory = (u8 *)gMachineMemory + 0x0200;

      gWindowsBitmapMemory =
          VirtualAlloc(0, kWindowWidth * kWindowHeight * sizeof(u32),
                       MEM_COMMIT, PAGE_READWRITE);

      // Init bitmap
      GlobalBitmapInfo.bmiHeader.biWidth = kWindowWidth;
      GlobalBitmapInfo.bmiHeader.biHeight = -kWindowHeight;
      GlobalBitmapInfo.bmiHeader.biSize = sizeof(GlobalBitmapInfo.bmiHeader);
      GlobalBitmapInfo.bmiHeader.biPlanes = 1;
      GlobalBitmapInfo.bmiHeader.biBitCount = 32;
      GlobalBitmapInfo.bmiHeader.biCompression = BI_RGB;

      // Run the machine
      HANDLE MainMachineThread = CreateThread(0, 0, MachineThread, 0, 0, 0);

      // Event loop
      while (gRunning) {
        // Process messages
        MSG Message;
        while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
          // Get keyboard messages
          switch (Message.message) {
            case WM_QUIT: {
              gRunning = false;
            } break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP: {
              u32 VKCode = (u32)Message.wParam;
              bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
              bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);

              bool32 AltKeyWasDown = (Message.lParam & (1 << 29));
              if ((VKCode == VK_F4) && AltKeyWasDown) {
                gRunning = false;
              }
              if (VKCode == VK_ESCAPE) {
                gRunning = false;
              }
            } break;

            default: {
              TranslateMessage(&Message);
              DispatchMessageA(&Message);
            } break;
          }
        }

        // TODO: sleep on vblank
        Win32UpdateWindow(hdc);
        Sleep(1);
      }
    }
  } else {
    // TODO: logging
    OutputDebugStringA("Couldn't register window class");
  }

  return 0;
}