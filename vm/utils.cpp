// ================== Utility functions ====================
#ifndef UTILS_CPP
#define UTILS_CPP

#include <ctype.h>

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

int strtoi(char *string, int length, int base) {
  int value = 0;
  int digit = 0;
  int power = 1;
  for (int i = 0; i < length; i++) {
    int c = tolower(string[length - i - 1]);
    if ('a' <= c && c <= 'f') {
      digit = 10 + (c - 'a');
    } else {
      digit = c - '0';
    }
    value += digit * power;
    power *= base;
  }
  return value;
}

#endif  // UTILS_CPP