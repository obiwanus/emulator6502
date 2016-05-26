#include "base.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>

/*************** TODO *****************

**************************************/

global bool gRunning;
global void *gLinuxBitmapMemory;

#include "vm.cpp"

global XImage *gXImage;

static void *machine_thread(void *arg) {
  CPU cpu = CPU();
  while (cpu.is_running) {
    cpu.Tick();
    usleep(1);
  }
  print("CPU has finished work\n");
}

int main(int argc, char const *argv[]) {
  Display *display;
  Window window;
  int screen;

  display = XOpenDisplay(0);
  if (display == 0) {
    fprintf(stderr, "Cannot open display\n");
    return 1;
  }

  screen = DefaultScreen(display);

  u32 border_color = WhitePixel(display, screen);
  u32 bg_color = BlackPixel(display, screen);

  window = XCreateSimpleWindow(display, RootWindow(display, screen), 300, 300,
                               kWindowWidth * SCREEN_ZOOM,
                               kWindowHeight * SCREEN_ZOOM, 0, border_color,
                               bg_color);

  XSetStandardProperties(display, window, "6502 virtual machine", "Hi!", None,
                         NULL, 0, NULL);

  XSelectInput(display, window, ExposureMask | KeyPressMask | KeyReleaseMask |
                                    ButtonPressMask | StructureNotifyMask);
  XMapRaised(display, window);

  Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(display, window, &wmDeleteMessage, 1);

  GC gc;
  XGCValues gcvalues;

  // Create x image
  {
    for (;;) {
      XEvent e;
      XNextEvent(display, &e);
      if (e.type == MapNotify) break;
    }

    gXImage = XGetImage(display, window, 0, 0, kWindowWidth * SCREEN_ZOOM,
                        kWindowHeight * SCREEN_ZOOM, AllPlanes, ZPixmap);

    gLinuxBitmapMemory = (void *)gXImage->data;

    gc = XCreateGC(display, window, 0, &gcvalues);
  }

  // Init VM memory
  gMachineMemory = malloc(kMachineMemorySize);
  gVideoMemory = (u8 *)gMachineMemory + 0x0200;

  // Load the program at $D400
  LoadProgram("test/pong.s", 0xD400);

  gRunning = true;

  // Run the machine
  pthread_t thread_id;
  if (pthread_create(&thread_id, 0, &machine_thread, 0) != 0) {
    fprintf(stderr, "Cannot create thread\n");
    return 1;
  }

  while (gRunning) {
    // Process events
    while (XPending(display)) {
      XEvent event;
      XNextEvent(display, &event);

      // Close window message
      if (event.type == ClientMessage) {
        if (event.xclient.data.l[0] == wmDeleteMessage) {
          gRunning = false;
        }
      }
    }

    // Copy data from the machine's video memory to our "display"
    // and stretch pixels
    for (int y = 0; y < kWindowHeight; y++) {
      for (int x = 0; x < kWindowWidth; x++) {
        u8 *src_pixel = gVideoMemory + kWindowWidth * y + x;
        u32 *dest_pixel =
            (u32 *)gLinuxBitmapMemory + (kWindowWidth * SCREEN_ZOOM * y + x) * SCREEN_ZOOM;
        u32 color = GetColor(*src_pixel);
        for (int py = 0; py < SCREEN_ZOOM; py++) {
          for (int px = 0; px < SCREEN_ZOOM; px++) {
            *(dest_pixel + py * kWindowWidth * SCREEN_ZOOM + px) = color;
          }
        }
      }
    }

    XPutImage(display, window, gc, gXImage, 0, 0, 0, 0, kWindowWidth * SCREEN_ZOOM,
              kWindowHeight * SCREEN_ZOOM);
  }

  XCloseDisplay(display);

  return 0;
}
