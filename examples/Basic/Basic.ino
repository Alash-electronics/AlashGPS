// [ru] AlashGPS: самый простой пример. Координаты, высота, скорость и число спутников раз в секунду.
//
//      Подключение модуля NEO-6M: VCC к 3.3 В или 5 В, GND к GND,
//      TX модуля к пину RX платы (Uno: D4, ESP32: GPIO16), RX модуля к пину TX платы (Uno: D3, ESP32: GPIO17).
//      Модуль надо вынести на улицу или к окну: в помещении спутников нет. Первый запуск до 10-15 минут.
// [kk] AlashGPS: ең қарапайым мысал. Координаттар, биіктік, жылдамдық және серіктер саны секунд сайын.
//
//      NEO-6M модулін қосу: VCC 3.3 В немесе 5 В-қа, GND GND-ға,
//      модуль TX тақтаның RX пиніне (Uno: D4, ESP32: GPIO16), модуль RX тақтаның TX пиніне (Uno: D3, ESP32: GPIO17).
//      Модульді далаға немесе терезеге шығарыңыз: бөлмеде серіктер көрінбейді. Алғашқы іске қосу 10-15 минутқа дейін.
// [en] AlashGPS: the simplest example. Coordinates, altitude, speed and the number of satellites once a second.
//
//      NEO-6M module wiring: VCC to 3.3 V or 5 V, GND to GND,
//      module TX to the board RX pin (Uno: D4, ESP32: GPIO16), module RX to the board TX pin (Uno: D3, ESP32: GPIO17).
//      Take the module outdoors or to a window: there are no satellites indoors. The first start takes up to 10-15 minutes.
#include <AlashGPS.h>

AlashGPS gps;

void setup() {
  Serial.begin(115200);
#if defined(ESP32)
  gps.begin(16, 17);                      // RX платы = GPIO16 (сюда TX модуля), TX платы = GPIO17
#elif defined(ESP8266)
  gps.begin(14, 12);                      // D5 (сюда TX модуля), D6
#else
  gps.begin(4, 3);                        // Uno / Nano: D4 (сюда TX модуля), D3
#endif
}

void loop() {
  if (gps.hasFix()) {
    Serial.print("Широта: ");   Serial.println(gps.lat(), 6);
    Serial.print("Долгота: ");  Serial.println(gps.lon(), 6);
    Serial.print("Высота: ");   Serial.print(gps.altitude());    Serial.println(" м");
    Serial.print("Скорость: "); Serial.print(gps.speedKmh());    Serial.println(" км/ч");
    Serial.print("Спутников: "); Serial.println(gps.satellites());
  } else {
    Serial.println(gps.statusText());     // объяснит, чего не хватает
  }
  Serial.println();
  delay(1000);
}
