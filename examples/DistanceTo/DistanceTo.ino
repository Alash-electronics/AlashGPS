// AlashGPS: расстояние и направление до точки. Здесь точка это Астана; подставьте свои координаты
// (их можно посмотреть на карте: нажмите на место и скопируйте широту и долготу).
#include <AlashGPS.h>

AlashGPS gps;
const double TARGET_LAT = 51.1694;
const double TARGET_LON = 71.4491;

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
    double meters = gps.distanceTo(TARGET_LAT, TARGET_LON);
    Serial.print("До цели: ");
    if (meters >= 1000) {
      Serial.print(meters / 1000.0, 1);
      Serial.print(" км");
    } else {
      Serial.print(meters, 0);
      Serial.print(" м");
    }
    Serial.print(", направление ");
    Serial.print(gps.bearingTo(TARGET_LAT, TARGET_LON), 0);
    Serial.println(" градусов от севера");
  } else {
    Serial.println(gps.statusText());
  }
  delay(1000);
}
