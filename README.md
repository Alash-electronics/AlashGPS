# AlashGPS

[Русский](#русский) | [Қазақша](#қазақша) | [English](#english)

---

## Русский

Простая библиотека для **GPS-модулей** (NEO-6M, NEO-7M, NEO-8M и любых с выходом NMEA): координаты, высота, скорость, время, расстояние до точки. Разбор NMEA встроен, **TinyGPS не нужен**. Сделана для уроков и наборов Alash Electronics.

```cpp
#include <AlashGPS.h>
AlashGPS gps;

void setup() { Serial.begin(115200); gps.begin(4, 3); }   // RX платы = D4 (сюда TX модуля), TX платы = D3
void loop() {
  if (gps.hasFix()) {
    Serial.println(gps.lat(), 6);
    Serial.println(gps.lon(), 6);
  } else {
    Serial.println(gps.statusText());   // объяснит, чего не хватает
  }
  delay(1000);
}
```

### Что умеет
- **`begin(rx, tx)` одной строкой:** сам запускает программный порт (Uno, Nano, ESP8266) или Serial2 (ESP32). Своя плата или порт: `gps.begin(Serial1)`.
- **Значения:** `lat()`, `lon()`, `altitude()`, `speedKmh()`, `course()`, `satellites()`, `satellitesInView()`, `hdop()`. Если координат нет, возвращается `NAN`, а не 0,0.
- **Точные координаты на Uno:** `latE7()` и `lonE7()` целыми числами (десятимиллионные доли градуса): `float` на Uno теряет метры.
- **Время и дата:** по Гринвичу и **местное**: `localHour()`, `localMinute()`... По умолчанию Казахстан (UTC+5), другой пояс `setTimezone(6)`. Переход через полночь и Новый год учтён.
- **Расстояние и направление до точки:** `distanceTo(lat, lon)`, `bearingTo(lat, lon)`.
- **Проверка данных:** каждая строка NMEA проверяется по контрольной сумме, свежесть координат (`hasFix()` ложь, если спутники пропали более 3 секунд назад).
- **Понятная диагностика** на русском, казахском и английском: `status()`, `statusText()`, `diagnose(Serial)` различают три случая: от модуля нет данных (провода), идёт мусор (скорость порта) или модуль просто ещё не нашёл спутники (выйдите на улицу, первый запуск до 10-15 минут).
- Работает на **Arduino AVR, ESP32, ESP8266**.

### Подключение (NEO-6M)
| Модуль | Uno / Nano | ESP32 |
|---|---|---|
| VCC | 5 В | 3.3 В |
| GND | GND | GND |
| TX | D4 | GPIO16 |
| RX | D3 | GPIO17 |

### Чем отличается от других библиотек
Мы посмотрели TinyGPS++ (Mikal Hart), TinyGPS, NeoGPS и Adafruit GPS:
- В TinyGPS++ нужно самому читать порт и вызывать `gps.encode(Serial.read())`, подключать SoftwareSerial; у нас всё в `begin(rx, tx)`.
- Нет часового пояса: время всегда по Гринвичу, а местное считает каждый сам. У нас `localHour()` и UTC+5.
- Если координат нет, не понятно почему (провода? скорость? спутники?). У нас `statusText()`.
- Адаптация на ESP32 требует отдельного порта; у нас тот же код.

### Примеры
`Basic`, `Clock`, `DistanceTo`, `Diagnose`, `ESP32_Serial2`.

### Тесты
Разбор NMEA на настоящих строках (пример Wikipedia), контрольная сумма, полушария, местное время через полночь, расстояния проверены без железа: `extras/test`.

---

## Қазақша

**GPS модульдеріне** (NEO-6M, NEO-7M, NEO-8M және NMEA шығысы бар кез келгені) арналған қарапайым кітапхана: координаттар, биіктік, жылдамдық, уақыт, нүктеге дейінгі қашықтық. NMEA талдауы кірістірілген, **TinyGPS қажет емес**. Alash Electronics сабақтары мен жинақтарына арналған.

```cpp
#include <AlashGPS.h>
AlashGPS gps;

void setup() { Serial.begin(115200); gps.begin(4, 3); }
void loop() {
  if (gps.hasFix()) {
    Serial.println(gps.lat(), 6);
    Serial.println(gps.lon(), 6);
  } else {
    Serial.println(gps.statusText());
  }
  delay(1000);
}
```

### Мүмкіндіктері
- **`begin(rx, tx)` бір жолмен.**
- **Мәндер:** `lat()`, `lon()`, `altitude()`, `speedKmh()`, `course()`, `satellites()`, `hdop()`. Координат болмаса `NAN` қайтарады.
- **Уақыт пен күн:** Гринвич бойынша және **жергілікті**: `localHour()`, `localMinute()`... Әдепкі Қазақстан (UTC+5).
- **Нүктеге қашықтық және бағыт:** `distanceTo(lat, lon)`, `bearingTo(lat, lon)`.
- **Түсінікті диагностика** орысша, қазақша, ағылшынша: `status()`, `statusText()`, `diagnose(Serial)`.
- **Arduino AVR, ESP32, ESP8266** тақталарында жұмыс істейді.

### Қосу (NEO-6M)
| Модуль | Uno / Nano | ESP32 |
|---|---|---|
| VCC | 5 В | 3.3 В |
| GND | GND | GND |
| TX | D4 | GPIO16 |
| RX | D3 | GPIO17 |

### Мысалдар
`Basic`, `Clock`, `DistanceTo`, `Diagnose`, `ESP32_Serial2`.

---

## English

A simple library for **GPS modules** (NEO-6M, NEO-7M, NEO-8M and anything with NMEA output): position, altitude, speed, time, distance to a point. NMEA parsing is built in, **no TinyGPS needed**. Made for classes and Alash Electronics kits.

```cpp
#include <AlashGPS.h>
AlashGPS gps;

void setup() { Serial.begin(115200); gps.begin(4, 3); }   // board RX = D4 (module TX goes here), board TX = D3
void loop() {
  if (gps.hasFix()) {
    Serial.println(gps.lat(), 6);
    Serial.println(gps.lon(), 6);
  } else {
    Serial.println(gps.statusText());   // says what is missing
  }
  delay(1000);
}
```

### Features
- **One-line `begin(rx, tx)`:** starts a software port (Uno, Nano, ESP8266) or Serial2 (ESP32). Your own port: `gps.begin(Serial1)`.
- **Values:** `lat()`, `lon()`, `altitude()`, `speedKmh()`, `course()`, `satellites()`, `satellitesInView()`, `hdop()`. `NAN` when there is no fix, not 0,0.
- **Precise coordinates on Uno:** `latE7()` and `lonE7()` as integers (1e-7 degrees): `float` on Uno loses meters.
- **Time and date:** UTC and **local**: `localHour()`, `localMinute()`... Kazakhstan (UTC+5) by default, `setTimezone(6)` for another zone. Midnight and New Year rollovers are handled.
- **Distance and bearing to a point:** `distanceTo(lat, lon)`, `bearingTo(lat, lon)`.
- **Checked data:** every NMEA sentence is verified with its checksum; `hasFix()` is false when satellites were lost more than 3 seconds ago.
- **Clear diagnostics** in Russian, Kazakh and English: `status()`, `statusText()`, `diagnose(Serial)` tell apart: no data (wiring), garbage (port speed) or no satellites yet (go outdoors, the first start can take 10-15 minutes).
- Works on **Arduino AVR, ESP32, ESP8266**.

### Wiring (NEO-6M)
| Module | Uno / Nano | ESP32 |
|---|---|---|
| VCC | 5 V | 3.3 V |
| GND | GND | GND |
| TX | D4 | GPIO16 |
| RX | D3 | GPIO17 |

### Examples
`Basic`, `Clock`, `DistanceTo`, `Diagnose`, `ESP32_Serial2`.

### License
MIT, Alash Electronics.
