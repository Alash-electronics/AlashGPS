// AlashGPS на ESP32: свой аппаратный порт Serial2. Так делают, если нужны другие пины или скорость.
// TX модуля -> GPIO16, RX модуля -> GPIO17.
#include <AlashGPS.h>

AlashGPS gps;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);   // скорость NEO-6M по умолчанию 9600
  gps.begin(Serial2);                        // передаём готовый порт
}

void loop() {
  if (gps.hasFix()) {
    Serial.print(gps.lat(), 6);
    Serial.print(", ");
    Serial.println(gps.lon(), 6);
  } else {
    Serial.println(gps.statusText());
  }
  delay(1000);
}
