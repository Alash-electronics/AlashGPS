// AlashGPS: нет координат? Скетч послушает модуль 3 секунды и скажет, что не так:
// нет данных, пришёл мусор или просто не видно спутников.
#include <AlashGPS.h>

AlashGPS gps;

void setup() {
  Serial.begin(115200);
  delay(500);
  gps.setLanguage(AlashGPS::LANG_RU);     // LANG_KK: қазақша, LANG_EN: English
#if defined(ESP32)
  gps.begin(16, 17);                      // RX платы = GPIO16 (сюда TX модуля), TX платы = GPIO17
#elif defined(ESP8266)
  gps.begin(14, 12);                      // D5 (сюда TX модуля), D6
#else
  gps.begin(4, 3);                        // Uno / Nano: D4 (сюда TX модуля), D3
#endif
}

void loop() {
  gps.diagnose(Serial);
  Serial.println();
}
