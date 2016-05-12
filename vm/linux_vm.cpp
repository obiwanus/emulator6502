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

* stretch bits

**************************************/

global bool gRunning;
global void *gLinuxBitmapMemory;

#include "vm.cpp"

global XImage *gXImage;

static void *machine_thread(void *arg) {
  while (gRunning) {
    MachineTick();
    usleep(1000);
  }
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

  Pixmap pixmap;
  GC gc;
  XGCValues gcvalues;

  // Create x image
  {
    for (;;) {
      XEvent e;
      XNextEvent(display, &e);
      if (e.type == MapNotify) break;
    }

    gXImage = XGetImage(display, window, 0, 0, kWindowWidth, kWindowHeight,
                        AllPlanes, ZPixmap);

    gLinuxBitmapMemory = (void *)gXImage->data;

    gc = XCreateGC(display, window, 0, &gcvalues);
  }

  // Init VM memory
  gMachineMemory = malloc(kMachineMemorySize);
  gVideoMemory = (u8 *)gMachineMemory + 0x0200;

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

    CopyPixels(gLinuxBitmapMemory, gVideoMemory);
    XPutImage(display, window, gc, gXImage, 0, 0, 0, 0, kWindowWidth,
              kWindowHeight);
  }

  XCloseDisplay(display);

  return 0;
}
