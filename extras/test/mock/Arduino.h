// Минимальная имитация Arduino.h для запуска тестов на компьютере (g++).
#pragma once
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using __FlashStringHelper = char;
#define F(x) ((const __FlashStringHelper *)(x))
inline uint32_t g_ms = 100000;
inline uint32_t millis() { return g_ms; }
class Print {
 public:
  void print(const char *s) { printf("%s", s); }
  void print(int v) { printf("%d", v); }
  void println(const char *s = "") { printf("%s\n", s); }
  void println(int v) { printf("%d\n", v); }
  void println(double v, int d = 2) { printf("%.*f\n", d, v); }
};
class Stream : public Print {
 public:
  const char *data = nullptr; size_t pos = 0;
  int available() { return data && data[pos] ? 1 : 0; }
  int read() { return data && data[pos] ? data[pos++] : -1; }
};
