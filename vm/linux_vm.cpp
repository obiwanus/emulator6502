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

global bool gRunning;
global void *gVideoMemory;

#include "vm.cpp"

global XImage *gXImage;

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

  const int kWindowWidth = SCREEN_WIDTH;
  const int kWindowHeight = SCREEN_HEIGHT;

  window = XCreateSimpleWindow(display, RootWindow(display, screen), 300, 300,
                               kWindowWidth, kWindowHeight, 0, border_color,
                               bg_color);

  XSetStandardProperties(display, window, "My Window", "Hi!", None, NULL, 0,
                         NULL);

  XSelectInput(display, window,
               ExposureMask | KeyPressMask | KeyReleaseMask |
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

    gVideoMemory = (void *)gXImage->data;

    u32 *Pixel = (u32 *)gXImage->data;
    for (int i = 0; i < kWindowWidth * 700; i++) {
      *Pixel = 0x00FF00FF;
      Pixel++;
    }

    gc = XCreateGC(display, window, 0, &gcvalues);
  }

  gRunning = true;

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

    MachineTick();


    XPutImage(display, window, gc, gXImage, 0, 0, 0, 0, kWindowWidth,
              kWindowHeight);
  }

  XCloseDisplay(display);

  return 0;
}
