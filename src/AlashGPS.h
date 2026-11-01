// AlashGPS: GPS-модули (NEO-6M, NEO-7M, NEO-8M и любые с выходом NMEA): широта, долгота, высота, скорость, время, спутники.
// Без зависимостей (разбор NMEA встроен). MIT License. Alash Electronics, https://github.com/Alash-electronics/AlashGPS
#ifndef ALASH_GPS_H
#define ALASH_GPS_H

#include <Arduino.h>
#if defined(ARDUINO_ARCH_AVR) || defined(ESP8266)
#include <SoftwareSerial.h>
#endif

class AlashGPS {
 public:
  ~AlashGPS();
  enum Language : uint8_t { LANG_RU = 0, LANG_KK = 1, LANG_EN = 2 };

  // Что с модулем (для status() и diagnose())
  enum Status : uint8_t {
    STATUS_NO_DATA = 0,     // от модуля не пришло ни одного символа: провода, питание
    STATUS_BAD_DATA,        // символы идут, но это мусор: скорость порта не 9600 или спутанные провода
    STATUS_NO_FIX,          // модуль работает, спутники ещё не найдены
    STATUS_FIX              // координаты есть
  };

  // ---- Запуск --------------------------------------------------------------
  // Вариант 1: свой порт (Serial1, Serial2, SoftwareSerial), который вы уже запустили на 9600.
  void begin(Stream &stream);
  // Вариант 2: сам запускает порт на пинах. rxPin это пин платы, куда идёт TX модуля.
  //   AVR и ESP8266: программный порт; ESP32: Serial2.
  bool begin(int8_t rxPin, int8_t txPin, uint32_t baud = 9600);

  // ---- Положение ---------------------------------------------------------------
  bool hasFix();                           // координаты определены и свежие
  double lat();                            // широта, градусы (юг со знаком минус); NAN без фикса
  double lon();                            // долгота, градусы (запад со знаком минус)
  int32_t latE7() const { return _lat; }   // то же в десятимиллионных долях градуса (точнее, чем float на Uno)
  int32_t lonE7() const { return _lon; }
  float altitude();                        // высота над уровнем моря, м
  float speedKmh();                        // скорость, км/ч
  float speedKnots();                      // скорость, узлы
  float course();                          // курс, градусы (0 = север); NAN, если стоим
  uint8_t satellites();                    // спутников в решении
  uint8_t satellitesInView();              // спутников в зоне видимости
  float hdop();                            // точность по горизонтали (чем меньше, тем лучше; до 2 хорошо)
  uint32_t fixAgeMs();                     // сколько мс назад обновлялись координаты

  // ---- Время и дата (по Гринвичу) ---------------------------------------------------
  bool hasTime();
  uint8_t hour();
  uint8_t minute();
  uint8_t second();
  uint8_t day();
  uint8_t month();
  uint16_t year();
  uint32_t unixTime();                     // секунды с 1970 года (UTC)

  // ---- Местное время (Казахстан: UTC+5) ---------------------------------------------
  void setTimezone(float hoursFromUtc) { _tzMin = (int16_t)(hoursFromUtc * 60.0f); }   // по умолчанию +5
  uint8_t localHour();
  uint8_t localMinute();
  uint8_t localSecond();
  uint8_t localDay();
  uint8_t localMonth();
  uint16_t localYear();

  // ---- Расстояния ----------------------------------------------------------------
  double distanceTo(double lat2, double lon2);   // до точки, метры (по дуге большого круга); NAN без фикса
  double bearingTo(double lat2, double lon2);    // направление на точку, градусы от севера; NAN без фикса
  static double distanceBetween(double lat1, double lon1, double lat2, double lon2);

  // ---- События ---------------------------------------------------------------------
  bool updated();                          // пришли новые координаты (true один раз)
  void update() { poll(); }                // читать порт вручную (getters делают это сами)
  void setTimeoutMs(uint16_t ms) { _timeoutMs = ms; }   // координаты считаются свежими столько мс (по умолчанию 3000)

  // ---- Диагностика -----------------------------------------------------------------
  void setLanguage(Language lang) { _lang = lang; }
  Status status();
  const __FlashStringHelper *statusText();
  void diagnose(Print &out);
  uint32_t charsReceived() const { return _chars; }
  uint16_t sentencesOk() const { return _ok; }
  uint16_t checksumErrors() const { return _bad; }

  // Подать символ вручную (для чтения из файла или тестов). Порт читается через poll().
  void feed(char c);

 private:
  Stream *_s = nullptr;
  Language _lang = LANG_RU;
  uint16_t _timeoutMs = 3000;
  int16_t _tzMin = 5 * 60;

  char _buf[84];
  uint8_t _len = 0;
  bool _inSentence = false;

  int32_t _lat = 0, _lon = 0;
  bool _hasPos = false, _hasTime = false, _hasDate = false;
  bool _rmcValid = false;
  uint8_t _fixQuality = 0, _sats = 0, _satsView = 0;
  int16_t _hdop100 = 0;
  float _alt = 0, _knots = 0, _course = 0;
  bool _hasCourse = false;
  uint8_t _h = 0, _m = 0, _sec = 0, _d = 0, _mo = 0;
  uint16_t _y = 0;
  uint32_t _lastFixAt = 0, _lastSentenceAt = 0;
  bool _updated = false;
  uint32_t _chars = 0;
  uint16_t _ok = 0, _bad = 0;

  void poll();
  void processSentence();
  bool parseTime(const char *f);
  static int32_t parseCoord(const char *f, char hemi);
  static float parseFloat(const char *f);
  static void civilFromDays(int32_t z, int &y, unsigned &m, unsigned &d);
  void local(uint8_t &h, uint8_t &mi, uint8_t &s, uint8_t &d, uint8_t &mo, uint16_t &y);
#if defined(ARDUINO_ARCH_AVR) || defined(ESP8266)
  SoftwareSerial *_soft = nullptr;
#endif
};

#endif  // ALASH_GPS_H
