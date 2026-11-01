// Тесты AlashGPS без железа: настоящие строки NMEA подаются как из порта.
//   g++ -std=c++17 -Wall -I mock -I ../../src test_gps.cpp ../../src/AlashGPS.cpp && ./a.out
#include <stdio.h>
#include <string>
#include "Arduino.h"
#include "AlashGPS.h"

static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); fails++; } } while (0)
static bool near(double a, double b, double eps) { return fabs(a - b) <= eps; }

// Собирает строку NMEA с правильной контрольной суммой
static std::string nmea(const std::string &body) {
  uint8_t sum = 0;
  for (char c : body) sum ^= (uint8_t)c;
  char tail[8];
  snprintf(tail, sizeof tail, "*%02X\r\n", sum);
  return "$" + body + tail;
}

int main() {
  // 1) Классический пример NMEA (Wikipedia): 48°07.038'N 011°31.000'E, 22.4 узла, курс 84.4, 23.03.1994 12:35:19
  AlashGPS g;
  Stream port;
  g.begin(port);
  std::string data = std::string("$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n") +
                     "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
  port.data = data.c_str();
  CHECK(g.hasFix());
  CHECK(near(g.lat(), 48.1173, 0.00001));
  CHECK(near(g.lon(), 11.516667, 0.00001));
  CHECK(g.latE7() == 481173000 && g.lonE7() == 115166666);
  CHECK(near(g.speedKnots(), 22.4, 0.01) && near(g.speedKmh(), 41.48, 0.01));
  CHECK(near(g.course(), 84.4, 0.01));
  CHECK(near(g.altitude(), 545.4, 0.01));
  CHECK(g.satellites() == 8 && near(g.hdop(), 0.9, 0.001));
  CHECK(g.hasTime() && g.hour() == 12 && g.minute() == 35 && g.second() == 19);
  CHECK(g.day() == 23 && g.month() == 3 && g.year() == 1994);
  CHECK(g.unixTime() == 764426119UL);                   // 1994-03-23 12:35:19 UTC
  CHECK(g.status() == AlashGPS::STATUS_FIX);
  CHECK(g.updated() && !g.updated());
  CHECK(g.sentencesOk() == 2 && g.checksumErrors() == 0);

  // 2) южное и западное полушария дают минус
  AlashGPS s;
  Stream p2; s.begin(p2);
  std::string south = nmea("GNRMC,010203.00,A,3351.4800,S,15112.6000,E,0.0,,010125,,,A");
  p2.data = south.c_str();
  CHECK(s.hasFix() && s.lat() < 0 && near(s.lat(), -33.858, 0.0001) && near(s.lon(), 151.21, 0.0001));
  CHECK(isnan(s.course()));                              // курса нет, и на месте он не определён
  std::string west = nmea("GPRMC,010203.00,A,4000.0000,N,07400.0000,W,5.0,90.0,010125,,,A");
  p2.data = west.c_str(); p2.pos = 0;
  CHECK(s.hasFix() && near(s.lon(), -74.0, 0.00001) && near(s.course(), 90.0, 0.01));

  // 3) испорченная контрольная сумма отбрасывается
  AlashGPS b;
  Stream p3; b.begin(p3);
  std::string bad = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*00\r\n";
  p3.data = bad.c_str();
  CHECK(!b.hasFix() && b.checksumErrors() == 1 && b.sentencesOk() == 0);

  // 4) нет фикса (статус V, пустые поля): координат нет, время может быть
  AlashGPS n;
  Stream p4; n.begin(p4);
  std::string nofix = nmea("GPRMC,235959.00,V,,,,,,,311224,,,N") + nmea("GPGGA,235959.00,,,,,0,00,99.99,,,,,,") + nmea("GPGSV,3,1,11,03,03,111,00,04,15,270,00,06,01,010,00,13,06,292,00");
  p4.data = nofix.c_str();
  CHECK(!n.hasFix() && isnan(n.lat()) && isnan(n.speedKmh()));
  CHECK(n.status() == AlashGPS::STATUS_NO_FIX);
  CHECK(n.satellitesInView() == 11 && n.satellites() == 0);
  CHECK(n.hasTime() && n.second() == 59);

  // 5) местное время Казахстана UTC+5: сутки и год переходят вперёд
  CHECK(n.localHour() == 4 && n.localMinute() == 59 && n.localSecond() == 59);   // 23:59:59 UTC 31.12.2024 -> 04:59:59 01.01.2025
  CHECK(n.localDay() == 1 && n.localMonth() == 1 && n.localYear() == 2025);
  n.setTimezone(-8);                                    // назад: 15:59:59 31.12.2024
  CHECK(n.localHour() == 15 && n.localDay() == 31 && n.localMonth() == 12 && n.localYear() == 2024);
  n.setTimezone(5.5);                                   // Индия: 05:29:59
  CHECK(n.localHour() == 5 && n.localMinute() == 29);

  // 6) свежесть координат: через 3 секунды без данных фикса нет
  g_ms += 2000; CHECK(g.hasFix());
  g_ms += 2000; CHECK(!g.hasFix());
  CHECK(g.fixAgeMs() >= 4000);
  g.setTimeoutMs(10000); CHECK(g.hasFix());

  // 7) расстояния: Алматы -> Астана около 970 км, обратный курс; точка на месте = 0
  double km = AlashGPS::distanceBetween(43.2389, 76.8897, 51.1694, 71.4491) / 1000.0;
  CHECK(near(km, 970, 5));
  CHECK(near(AlashGPS::distanceBetween(10, 20, 10, 20), 0, 0.001));
  AlashGPS d;
  Stream p6; d.begin(p6);
  std::string almaty = nmea("GPRMC,060000,A,4314.3340,N,07653.3820,E,0.0,0.0,010125,,,A");
  p6.data = almaty.c_str();
  CHECK(near(d.distanceTo(51.1694, 71.4491) / 1000.0, 970, 6));
  double brg = d.bearingTo(51.1694, 71.4491);
  CHECK(near(brg, 337.0, 1.0));                         // Астана на северо-северо-западе
  CHECK(near(d.bearingTo(44.2389, 76.8897), 0, 1) || near(d.bearingTo(44.2389, 76.8897), 360, 1));   // строго на север

  // 8) диагностика: нет данных, мусор
  AlashGPS e;
  Stream p7; e.begin(p7);
  CHECK(e.status() == AlashGPS::STATUS_NO_DATA);
  std::string junk(300, 'x');
  p7.data = junk.c_str();
  CHECK(e.status() == AlashGPS::STATUS_BAD_DATA);

  // 9) посторонние строки (PUBX, TXT) игнорируются, длинная строка не ломает буфер
  AlashGPS o;
  Stream p8; o.begin(p8);
  std::string mixed = nmea("GPTXT,01,01,02,ANTSTATUS=OPEN") + "$" + std::string(200, 'A') + "\r\n" + nmea("GPRMC,010101,A,5500.0000,N,03700.0000,E,1.0,10.0,010125,,,A");
  p8.data = mixed.c_str();
  CHECK(o.hasFix() && near(o.lat(), 55.0, 0.00001));

  // 10) тексты
  e.setLanguage(AlashGPS::LANG_KK);
  p7.pos = 0;
  g_ms += 0;
  CHECK(strlen((const char *)e.statusText()) > 10);

  if (fails) { printf("FAILED: %d\n", fails); return 1; }
  printf("ALL TESTS PASSED\n");
  return 0;
}
