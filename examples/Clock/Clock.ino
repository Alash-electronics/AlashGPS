// [ru] AlashGPS: точные часы по спутникам. Время приходит от GPS, часовой пояс по умолчанию Казахстан (UTC+5).
//      Для другого пояса: gps.setTimezone(6); или gps.setTimezone(5.5);
// [kk] AlashGPS: серіктерден дәл сағат. Уақыт GPS-тен келеді, әдепкі уақыт белдеуі Қазақстан (UTC+5).
//      Басқа белдеу үшін: gps.setTimezone(6); немесе gps.setTimezone(5.5);
// [en] AlashGPS: accurate clock from satellites. The time comes from GPS, the default time zone is Kazakhstan (UTC+5).
//      For another zone: gps.setTimezone(6); or gps.setTimezone(5.5);
#include <AlashGPS.h>

AlashGPS gps;

void print2(int v) {
  if (v < 10) Serial.print('0');
  Serial.print(v);
}

void setup() {
  Serial.begin(115200);
#if defined(ESP32)
  gps.begin(16, 17);                      // RX платы = GPIO16 (сюда TX модуля), TX платы = GPIO17
#elif defined(ESP8266)
  gps.begin(14, 12);                      // D5 (сюда TX модуля), D6
#else
  gps.begin(4, 3);                        // Uno / Nano: D4 (сюда TX модуля), D3
#endif
  gps.setTimezone(5);
}

void loop() {
  if (gps.hasTime()) {
    print2(gps.localHour());   Serial.print(':');
    print2(gps.localMinute()); Serial.print(':');
    print2(gps.localSecond());
    Serial.print("   ");
    print2(gps.localDay());    Serial.print('.');
    print2(gps.localMonth());  Serial.print('.');
    Serial.println(gps.localYear());
  } else {
    Serial.println("Жду время от спутников...");
  }
  delay(1000);
}
