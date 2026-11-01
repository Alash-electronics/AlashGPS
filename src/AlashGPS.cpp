// MIT License. Alash Electronics.
#include "AlashGPS.h"

#define T(ru, kk, en) (_lang == LANG_KK ? F(kk) : _lang == LANG_EN ? F(en) : F(ru))

AlashGPS::~AlashGPS() {
#if defined(ARDUINO_ARCH_AVR) || defined(ESP8266)
  delete _soft;
#endif
}

void AlashGPS::begin(Stream &stream) { _s = &stream; }

bool AlashGPS::begin(int8_t rxPin, int8_t txPin, uint32_t baud) {
#if defined(ESP32)
  Serial2.begin(baud, SERIAL_8N1, rxPin, txPin);
  _s = &Serial2;
  return true;
#elif defined(ARDUINO_ARCH_AVR) || defined(ESP8266)
  if (_soft) delete _soft;
  _soft = new SoftwareSerial(rxPin, txPin);
  _soft->begin(baud);
  _s = _soft;
  return true;
#else
  (void)rxPin; (void)txPin; (void)baud;
  return false;                            // на этой плате запустите порт сами и передайте его в begin(Serial1)
#endif
}

void AlashGPS::poll() {
  if (!_s) return;
  for (uint16_t n = 0; n < 512 && _s->available(); n++) feed((char)_s->read());
}

// ---- Разбор NMEA -------------------------------------------------------------------

void AlashGPS::feed(char c) {
  _chars++;
  if (c == '$') {
    _len = 0;
    _inSentence = true;
    return;
  }
  if (!_inSentence) return;
  if (c == '\n' || c == '\r') {
    if (_len) {
      _buf[_len] = 0;
      processSentence();
    }
    _inSentence = false;
    _len = 0;
    return;
  }
  if (_len < sizeof(_buf) - 2) _buf[_len++] = c;
  else _inSentence = false;                // слишком длинная строка: мусор
}

static int hexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

float AlashGPS::parseFloat(const char *f) {
  if (!f || !*f) return NAN;
  return (float)atof(f);
}

// "ddmm.mmmm" + N/S/E/W -> десятимиллионные доли градуса, чтобы не терять точность на float
int32_t AlashGPS::parseCoord(const char *f, char hemi) {
  int32_t intPart = 0;
  const char *p = f;
  while (*p >= '0' && *p <= '9') intPart = intPart * 10 + (*p++ - '0');
  int64_t frac = 0;                        // дробная часть минут, ровно 7 знаков
  uint8_t digits = 0;
  if (*p == '.') {
    p++;
    while (*p >= '0' && *p <= '9' && digits < 7) {
      frac = frac * 10 + (*p++ - '0');
      digits++;
    }
  }
  while (digits++ < 7) frac *= 10;
  int64_t minutesE7 = (int64_t)(intPart % 100) * 10000000LL + frac;
  int64_t degE7 = (int64_t)(intPart / 100) * 10000000LL + minutesE7 / 60;
  if (hemi == 'S' || hemi == 'W') degE7 = -degE7;
  return (int32_t)degE7;
}

bool AlashGPS::parseTime(const char *f) {
  if (!f || strlen(f) < 6) return false;
  for (uint8_t i = 0; i < 6; i++)
    if (f[i] < '0' || f[i] > '9') return false;
  _h = (f[0] - '0') * 10 + (f[1] - '0');
  _m = (f[2] - '0') * 10 + (f[3] - '0');
  _sec = (f[4] - '0') * 10 + (f[5] - '0');
  _hasTime = _h < 24 && _m < 60 && _sec < 61;
  return _hasTime;
}

void AlashGPS::processSentence() {
  // Контрольная сумма: XOR всех символов между $ и *
  char *star = strchr(_buf, '*');
  if (!star) { _bad++; return; }
  uint8_t sum = 0;
  for (char *p = _buf; p < star; p++) sum ^= (uint8_t)*p;
  int hi = hexVal(star[1]), lo = hexVal(star[2]);
  if (hi < 0 || lo < 0 || sum != (uint8_t)(hi * 16 + lo)) { _bad++; return; }
  *star = 0;
  _ok++;
  _lastSentenceAt = millis();

  // Делим на поля по запятым (на месте)
  char *f[20];
  uint8_t n = 0;
  f[n++] = _buf;
  for (char *p = _buf; *p && n < 20; p++)
    if (*p == ',') { *p = 0; f[n++] = p + 1; }
  if (strlen(f[0]) < 5) return;
  const char *type = f[0] + 2;             // без «GP» / «GN» / «GL»

  if (strncmp(type, "RMC", 3) == 0 && n >= 10) {
    parseTime(f[1]);
    _rmcValid = (f[2][0] == 'A');
    if (_rmcValid) {
      _lat = parseCoord(f[3], f[4][0]);
      _lon = parseCoord(f[5], f[6][0]);
      _hasPos = true;
      _knots = parseFloat(f[7]);
      if (isnan(_knots)) _knots = 0;
      float c = parseFloat(f[8]);
      _hasCourse = !isnan(c);
      if (_hasCourse) _course = c;
      _lastFixAt = millis();
      _updated = true;
    }
    if (strlen(f[9]) == 6) {
      _d = (f[9][0] - '0') * 10 + (f[9][1] - '0');
      _mo = (f[9][2] - '0') * 10 + (f[9][3] - '0');
      uint8_t yy = (f[9][4] - '0') * 10 + (f[9][5] - '0');
      _y = yy >= 80 ? 1900 + yy : 2000 + yy;   // GPS появился в 1980: 80-99 это 1980-1999, 00-79 это 2000-2079
      _hasDate = _d >= 1 && _d <= 31 && _mo >= 1 && _mo <= 12;
    }
  } else if (strncmp(type, "GGA", 3) == 0 && n >= 10) {
    parseTime(f[1]);
    _fixQuality = (uint8_t)atoi(f[6]);
    _sats = (uint8_t)atoi(f[7]);
    float h = parseFloat(f[8]);
    _hdop100 = isnan(h) ? 0 : (int16_t)(h * 100.0f + 0.5f);
    if (_fixQuality > 0) {
      _lat = parseCoord(f[2], f[3][0]);
      _lon = parseCoord(f[4], f[5][0]);
      _hasPos = true;
      float a = parseFloat(f[9]);
      if (!isnan(a)) _alt = a;
      _lastFixAt = millis();
      _updated = true;
    }
  } else if (strncmp(type, "GSV", 3) == 0 && n >= 4) {
    _satsView = (uint8_t)atoi(f[3]);
  }
}

// ---- Значения -------------------------------------------------------------------------

bool AlashGPS::hasFix() {
  poll();
  return _hasPos && (_rmcValid || _fixQuality > 0) && (uint32_t)(millis() - _lastFixAt) <= _timeoutMs;
}

double AlashGPS::lat() { return hasFix() ? _lat / 1e7 : NAN; }
double AlashGPS::lon() { return hasFix() ? _lon / 1e7 : NAN; }
float AlashGPS::altitude() { return hasFix() ? _alt : NAN; }
float AlashGPS::speedKnots() { return hasFix() ? _knots : NAN; }
float AlashGPS::speedKmh() { return hasFix() ? _knots * 1.852f : NAN; }
float AlashGPS::course() { return hasFix() && _hasCourse && _knots > 0.5f ? _course : NAN; }   // на месте курс не определён
uint8_t AlashGPS::satellites() { poll(); return _sats; }
uint8_t AlashGPS::satellitesInView() { poll(); return _satsView; }
float AlashGPS::hdop() { poll(); return _hdop100 ? _hdop100 / 100.0f : NAN; }
uint32_t AlashGPS::fixAgeMs() { poll(); return _hasPos ? (uint32_t)(millis() - _lastFixAt) : 0xFFFFFFFFu; }
bool AlashGPS::updated() { poll(); bool u = _updated; _updated = false; return u; }

bool AlashGPS::hasTime() { poll(); return _hasTime && _hasDate; }
uint8_t AlashGPS::hour() { poll(); return _h; }
uint8_t AlashGPS::minute() { poll(); return _m; }
uint8_t AlashGPS::second() { poll(); return _sec; }
uint8_t AlashGPS::day() { poll(); return _d; }
uint8_t AlashGPS::month() { poll(); return _mo; }
uint16_t AlashGPS::year() { poll(); return _y; }

// Дни от 1970-01-01 для даты и обратно (алгоритм Говарда Хиннанта, работает для всех дат)
static int32_t daysFromCivil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  int era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (int32_t)doe - 719468;
}

void AlashGPS::civilFromDays(int32_t z, int &y, unsigned &m, unsigned &d) {
  z += 719468;
  int era = (z >= 0 ? z : z - 146096) / 146097;
  unsigned doe = (unsigned)(z - era * 146097);
  unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  y = (int)yoe + era * 400;
  unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  unsigned mp = (5 * doy + 2) / 153;
  d = doy - (153 * mp + 2) / 5 + 1;
  m = mp < 10 ? mp + 3 : mp - 9;
  y += m <= 2;
}

uint32_t AlashGPS::unixTime() {
  if (!hasTime()) return 0;
  return (uint32_t)daysFromCivil(_y, _mo, _d) * 86400UL + _h * 3600UL + _m * 60UL + _sec;
}

void AlashGPS::local(uint8_t &h, uint8_t &mi, uint8_t &s, uint8_t &d, uint8_t &mo, uint16_t &y) {
  h = mi = s = d = mo = 0;
  y = 0;
  if (!hasTime()) return;
  int32_t total = (int32_t)(_h * 3600L + _m * 60L + _sec) + (int32_t)_tzMin * 60L;
  int32_t days = daysFromCivil(_y, _mo, _d);
  while (total < 0) { total += 86400; days--; }
  while (total >= 86400) { total -= 86400; days++; }
  int yy; unsigned mm, dd;
  civilFromDays(days, yy, mm, dd);
  h = total / 3600; mi = (total % 3600) / 60; s = total % 60;
  d = dd; mo = mm; y = yy;
}

uint8_t AlashGPS::localHour() { uint8_t h, mi, s, d, mo; uint16_t y; local(h, mi, s, d, mo, y); return h; }
uint8_t AlashGPS::localMinute() { uint8_t h, mi, s, d, mo; uint16_t y; local(h, mi, s, d, mo, y); return mi; }
uint8_t AlashGPS::localSecond() { uint8_t h, mi, s, d, mo; uint16_t y; local(h, mi, s, d, mo, y); return s; }
uint8_t AlashGPS::localDay() { uint8_t h, mi, s, d, mo; uint16_t y; local(h, mi, s, d, mo, y); return d; }
uint8_t AlashGPS::localMonth() { uint8_t h, mi, s, d, mo; uint16_t y; local(h, mi, s, d, mo, y); return mo; }
uint16_t AlashGPS::localYear() { uint8_t h, mi, s, d, mo; uint16_t y; local(h, mi, s, d, mo, y); return y; }

// ---- Расстояния --------------------------------------------------------------------------

double AlashGPS::distanceBetween(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0, rad = 0.017453292519943295;
  double p1 = lat1 * rad, p2 = lat2 * rad, dp = (lat2 - lat1) * rad, dl = (lon2 - lon1) * rad;
  double a = sin(dp / 2) * sin(dp / 2) + cos(p1) * cos(p2) * sin(dl / 2) * sin(dl / 2);
  return R * 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
}

double AlashGPS::distanceTo(double lat2, double lon2) {
  if (!hasFix()) return NAN;
  return distanceBetween(_lat / 1e7, _lon / 1e7, lat2, lon2);
}

double AlashGPS::bearingTo(double lat2, double lon2) {
  if (!hasFix()) return NAN;
  const double rad = 0.017453292519943295;
  double p1 = (_lat / 1e7) * rad, p2 = lat2 * rad, dl = (lon2 - _lon / 1e7) * rad;
  double y = sin(dl) * cos(p2), x = cos(p1) * sin(p2) - sin(p1) * cos(p2) * cos(dl);
  double b = atan2(y, x) / rad;
  return b < 0 ? b + 360.0 : b;
}

// ---- Диагностика -------------------------------------------------------------------------

AlashGPS::Status AlashGPS::status() {
  poll();
  if (_chars == 0) return STATUS_NO_DATA;
  if (_ok == 0) return _chars >= 200 ? STATUS_BAD_DATA : STATUS_NO_DATA;
  return hasFix() ? STATUS_FIX : STATUS_NO_FIX;
}

const __FlashStringHelper *AlashGPS::statusText() {
  switch (status()) {
    case STATUS_NO_DATA:
      return T("Нет данных от модуля: TX модуля соедините с RX платы, проверьте питание (3.3-5 В) и GND",
               "Модульден дерек жоқ: модуль TX-ін тақта RX-іне қосыңыз, қуатты (3.3-5 В) және GND тексеріңіз",
               "No data from the module: connect the module TX to the board RX, check power (3.3-5 V) and GND");
    case STATUS_BAD_DATA:
      return T("Приходит мусор: проверьте скорость порта (у NEO-6M 9600) и что TX модуля идёт на RX платы, а не на TX",
               "Қоқыс келіп жатыр: порт жылдамдығын (NEO-6M үшін 9600) және модуль TX-і тақта RX-іне қосылғанын тексеріңіз",
               "Garbage is coming in: check the port speed (NEO-6M uses 9600) and that the module TX goes to the board RX, not TX");
    case STATUS_NO_FIX:
      return T("Модуль работает, но спутников пока нет: выйдите на открытое место (не в помещении). Первый запуск до 10-15 минут",
               "Модуль жұмыс істейді, бірақ жолдас әлі жоқ: ашық жерге шығыңыз (бөлмеде емес). Алғашқы іске қосу 10-15 минутқа дейін",
               "The module works but has no satellites yet: go outdoors with a clear sky. The first start can take up to 10-15 minutes");
    case STATUS_FIX:
      return T("Координаты определены", "Координаттар анықталды", "Position fixed");
  }
  return F("");
}

void AlashGPS::diagnose(Print &out) {
  out.println(T("AlashGPS: диагностика (слушаю модуль 3 секунды)", "AlashGPS: диагностика (модульді 3 секунд тыңдаймын)", "AlashGPS: diagnostics (listening to the module for 3 seconds)"));
  uint32_t until = millis() + 3000;
  while ((int32_t)(millis() - until) < 0) poll();
  out.print(T("Получено символов: ", "Алынған таңбалар: ", "Characters received: "));
  out.println((int)_chars);
  out.print(T("Верных строк NMEA: ", "Дұрыс NMEA жолдары: ", "Valid NMEA sentences: "));
  out.println((int)_ok);
  out.print(T("С ошибкой контрольной суммы: ", "Бақылау сомасы қате: ", "Checksum errors: "));
  out.println((int)_bad);
  out.print(T("Спутников в решении / в зоне видимости: ", "Шешімдегі / көрінетін жолдастар: ", "Satellites used / in view: "));
  out.print((int)_sats);
  out.print(F(" / "));
  out.println((int)_satsView);
  out.println(statusText());
  if (status() == STATUS_FIX) {
    out.print(T("Широта: ", "Ендік: ", "Latitude: "));
    out.println((double)_lat / 1e7, 6);
    out.print(T("Долгота: ", "Бойлық: ", "Longitude: "));
    out.println((double)_lon / 1e7, 6);
  }
}
