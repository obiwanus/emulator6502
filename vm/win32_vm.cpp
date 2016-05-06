#include "base.h"

global void *gMachineMemory;
global void *gVideoMemory;
global volatile bool32 gRunning;

#include "vm.cpp"

#include <windows.h>
#include <intrin.h>

global BITMAPINFO GlobalBitmapInfo;

internal void Win32UpdateWindow(HDC hdc) {
  if (!gVideoMemory) return;

  // Copy data from the machine's video memory to our "display"
  u8 *LeftTopSrcPixel = (u8 *)gMachineMemory + 0x0200;
  u32 *LeftTopDestPixel = (u32 *)gVideoMemory;
  int BytesPerPixel = 4;
  int Pitch = SCREEN_WIDTH * BytesPerPixel;

  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      u8 *SrcPixel = LeftTopSrcPixel + SCREEN_WIDTH * y + x;
      u32 *DestPixel = LeftTopDestPixel + Pitch * y + x;  // TODO: PIXEL_SIZE > 1
      u32 Value = 0;
      switch (*SrcPixel) {
        case 0: {
          Value = 0x00000000;  // black
        } break;
        case 1: {
          Value = 0x00FF0000;  // red
        } break;
        case 2: {
          Value = 0x0000FF00;  // green
        } break;
        case 3: {
          Value = 0x000000FF;  // blue
        } break;
        default: {
          Value = 0x00CCCCCC;  // grey
        } break;
      }
      *DestPixel = Value;
    }
  }

  StretchDIBits(hdc, 0, 0, kWindowWidth, kWindowHeight,  // dest
                0, 0, kWindowWidth, kWindowHeight,       // src
                gVideoMemory, &GlobalBitmapInfo, DIB_RGB_COLORS, SRCCOPY);
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
    Sleep(5);
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
    HWND Window =
        CreateWindow(WindowClass.lpszClassName, 0,
                     WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME | WS_VISIBLE,
                     CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_WIDTH * PIXEL_SIZE,
                     SCREEN_HEIGHT * PIXEL_SIZE, 0, 0, hInstance, 0);

    if (Window) {
      // We're not going to release it as we use CS_OWNDC
      HDC hdc = GetDC(Window);

      // Real window size
      RECT WindowRect = {};
      GetClientRect(Window, &WindowRect);
      kWindowWidth = WindowRect.right;
      kWindowHeight = WindowRect.bottom;

      gRunning = true;

      // Init memory
      gMachineMemory = VirtualAlloc(0, MEMORY_SIZE, MEM_COMMIT, PAGE_READWRITE);


      // TODO: Find out why not enough memory!


      gVideoMemory = VirtualAlloc(0, 10 * SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u32),
                                  MEM_COMMIT, PAGE_READWRITE);

      // Init bitmap
      GlobalBitmapInfo.bmiHeader.biWidth = SCREEN_WIDTH;
      GlobalBitmapInfo.bmiHeader.biHeight = SCREEN_HEIGHT;
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
            } break;

            default: {
              TranslateMessage(&Message);
              DispatchMessageA(&Message);
            } break;
          }
        }

        // TODO: sleep on vblank
        Win32UpdateWindow(hdc);
        Sleep(5);
      }
    }
  } else {
    // TODO: logging
    OutputDebugStringA("Couldn't register window class");
  }

  return 0;
}