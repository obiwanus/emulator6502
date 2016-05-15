// ================== Utility functions ====================

#ifndef UTILS_CPP
#define UTILS_CPP

void Win32Print(char *String);

#ifdef BUILD_WIN32
#define print(...)                      \
  {                                     \
    char pbuf[2000];                    \
    sprintf_s(pbuf, 2000, __VA_ARGS__); \
    Win32Print(pbuf);                   \
  }
#else
  // TODO: be able to choose stdout/stderr
#define print(...) (printf(__VA_ARGS__))
#endif

#endif  // UTILS_CPP