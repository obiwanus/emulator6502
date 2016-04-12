#include "base.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // usleep
#include <time.h>
#include <limits.h>

global bool gRunning;

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

  const int kWindowWidth = 1500;
  const int kWindowHeight = 1000;

  window = XCreateSimpleWindow(display, RootWindow(display, screen), 300, 300,
                               kWindowWidth, kWindowHeight, 0, border_color,
                               bg_color);

  XSetStandardProperties(display, window, "My Window", "Hi!", None, NULL, 0,
                         NULL);

  XSelectInput(display, window,
               ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask);
  XMapRaised(display, window);

  Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(display, window, &wmDeleteMessage, 1);

  usleep(5000);  // 50 ms

  // Init backbuffer
  GameBackBuffer.MaxWidth = 2000;
  GameBackBuffer.MaxHeight = 1500;
  GameBackBuffer.BytesPerPixel = 4;

  // @tmp
  GameBackBuffer.Width = kWindowWidth;
  GameBackBuffer.Height = kWindowHeight;

  int BufferSize = GameBackBuffer.MaxWidth * GameBackBuffer.MaxHeight *
                   GameBackBuffer.BytesPerPixel;

  GameBackBuffer.Memory = malloc(BufferSize);

  Pixmap pixmap;
  GC gc;
  XGCValues gcvalues;

  // Create x image
  {
    // int depth = 32;
    // int bitmap_pad = 32;
    // int bytes_per_line = 0;
    // int offset = 0;

    // gXImage = XCreateImage(display, CopyFromParent, depth, ZPixmap, offset,
    //                        (char *)GameBackBuffer.Memory, kWindowWidth,
    //                        kWindowHeight, bitmap_pad, bytes_per_line);

    // TODO: find a way to do it with a newly created image
    usleep(500000);
    gXImage = XGetImage(display, window, 0, 0, kWindowWidth, kWindowHeight,
                        AllPlanes, ZPixmap);

    GameBackBuffer.Memory = (void *)gXImage->data;

    // u32 *Pixel = (u32 *)gXImage->data;
    // for (int i = 0; i < kWindowWidth * 700; i++) {
    //   *Pixel = 0x00FF00FF;
    //   Pixel++;
    // }

    // pixmap = XCreatePixmap(display, window, kWindowWidth,
    //                        kWindowHeight, depth);
    gc = XCreateGC(display, window, 0, &gcvalues);
  }

  gRunning = true;

  while (gRunning) {
    // Process events
    while (XPending(display)) {
      XEvent event;
      KeySym key;
      char buf[256];
      player_input *Player1 = &NewInput->Player1;
      player_input *Player2 = &NewInput->Player2;
      char symbol = 0;
      bool32 pressed = false;
      bool32 released = false;
      bool32 retriggered = false;

      XNextEvent(display, &event);

      if (XLookupString(&event.xkey, buf, 255, &key, 0) == 1) {
        symbol = buf[0];
      }

      // Process user input
      if (event.type == KeyPress) {
        printf("Key pressed\n");
        pressed = true;
      }

      if (event.type == KeyRelease) {
        if (XEventsQueued(display, QueuedAfterReading)) {
          XEvent nev;
          XPeekEvent(display, &nev);

          if (nev.type == KeyPress && nev.xkey.time == event.xkey.time &&
              nev.xkey.keycode == event.xkey.keycode) {
            // Ignore. Key wasn't actually released
            printf("Key release ignored\n");
            XNextEvent(display, &event);
            retriggered = true;
          }
        }

        if (!retriggered) {
          printf("Key released\n");
          released = true;
        }
      }

      if (pressed || released) {
        if (key == XK_Escape) {
          gRunning = false;
        }

      // Close window message
      if (event.type == ClientMessage) {
        if (event.xclient.data.l[0] == wmDeleteMessage) {
          gRunning = false;
        }
      }
    }

    bool32 RedrawLevel = false;

    Game.UpdateAndRender(NewInput, &GameBackBuffer, &GameMemory, &gSoundOutput,
                         RedrawLevel);

    XPutImage(display, window, gc, gXImage, 0, 0, 0, 0, kWindowWidth,
              kWindowHeight);
  }

  XCloseDisplay(display);

  return 0;
}
