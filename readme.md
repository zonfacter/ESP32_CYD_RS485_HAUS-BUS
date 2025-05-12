# ESP32 Touch-Menü mit TFT Display

Dieses Projekt implementiert ein interaktives Touch-Menü für TZT ESP32 2.4" LCD mit ST7789 Controller und XPT2046 Touchscreen. Es bietet ein 6-Tasten-Interface, PWM-gesteuerte Hintergrundbeleuchtung, RGB-LED-Feedback und UART-Kommunikation.

## Hardware

- **Board**: TZT ESP32 2.4" LCD mit Touchscreen
- **Display-Controller**: ST7789
- **Touchscreen-Controller**: XPT2046
- **Prozessor**: ESP32

## Funktionen

- 6-Tasten Touch-Interface im 3x2 (Landscape) oder 2x3 (Portrait) Layout
- PWM-gesteuerte Hintergrundbeleuchtung
- RGB-LED für visuelle Rückmeldung bei Kommunikation
- UART-Kommunikation mit externen Geräten
- Regelmäßige Statusmeldungen

## Wichtige Einstellungen

### Display-Konfiguration (config.h)

```cpp
// Display-Dimensionen und Ausrichtung
#define PORTRAIT 0
#define LANDSCAPE 1
#define SCREEN_ORIENTATION LANDSCAPE  // Wechsel zu PORTRAIT für Hochformat

// Hintergrundbeleuchtungs-Pin und Kanal
#define TFT_BL_PIN 27
#define TFT_BL_CHANNEL 0  // PWM-Kanal
#define DEFAULT_BACKLIGHT 100  // Standardhelligkeit (0-100%)
```

### RGB-LED (config.h)

```cpp
// RGB-LED Pins
#define LED_RED_PIN 4
#define LED_GREEN_PIN 16
#define LED_BLUE_PIN 17

// LED-Blinkzeiten
#define LED_SEND_DURATION 100  // Dauer des roten Signals beim Senden (ms)
#define LED_RECEIVE_DURATION 20  // Dauer des blauen Signals beim Empfangen (ms)
```

**Hinweis**: Die RGB-LED arbeitet mit negierter Logik - LOW schaltet die LED ein, HIGH schaltet sie aus.

### UART-Kommunikation (config.h)

```cpp
// UART Pins für die Hardware-Serielle Kommunikation
#define UART_RX_PIN 22  // RX Pin
#define UART_TX_PIN 21  // TX Pin
#define UART_EN_PIN 35  // Enable Pin

// Kommunikationsprotokoll Konstanten
#define START_BYTE 0xFD  // ý
#define END_BYTE 0xFE    // þ
#define DEVICE_ID "5999"  // Geräte-ID
```

### Touch-Kalibrierung (config.h)

```cpp
// Touchscreen Pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 13  // T_DIN
#define XPT2046_MISO 12  // T_OUT
#define XPT2046_CLK 14   // T_CLK
#define XPT2046_CS 33    // T_CS

// Touch-Parameter für Kalibrierung
#define TOUCH_MIN_X 400
#define TOUCH_MAX_X 3900
#define TOUCH_MIN_Y 400
#define TOUCH_MAX_Y 3900

// Touch-Invertierung (kann im TEST-Modus angepasst werden)
extern bool invertTouchX;  // Standardmäßig true
extern bool invertTouchY;  // Standardmäßig true
```

## Kommunikationsprotokolle

### Telegramm-Format

```
[START_BYTE] DEVICE_ID.FUNCTION.INSTANCE_ID.ACTION.PARAMS [END_BYTE]
```

Beispiel: `ý 5999.LBN.1.SET_MBR.50 þ` setzt die Hintergrundbeleuchtung auf 50%.

### Verfügbare Befehle

| Befehl | Beschreibung | Beispiel |
|--------|--------------|----------|
| LBN.1.SET_MBR | Helligkeit einstellen (0-100%) | `ý 5999.LBN.1.SET_MBR.75 þ` |
| LED.x.1 | Button x aktivieren (grün) | `ý 5999.LED.2.1 þ` |
| LED.x.0 | Button x deaktivieren (grau) | `ý 5999.LED.2.0 þ` |
| BTN.x.STATUS.1 | Button x gedrückt (wird gesendet) | `ý 5999.BTN.3.STATUS.1 þ` |
| BTN.x.STATUS.0 | Button x losgelassen (wird gesendet) | `ý 5999.BTN.3.STATUS.0 þ` |

## UART-Konfiguration

- **Baudrate**: 115200
- **Format**: 8N1 (8 Datenbits, keine Parität, 1 Stoppbit)
- **Hardware-Verbindung**:
  - ESP32 TX (Pin 21) → Externes Gerät RX
  - ESP32 RX (Pin 22) → Externes Gerät TX
  - ESP32 GND → Externes Gerät GND

## Testmodus

Ein Testmodus zur Kalibrierung des Touchscreens kann durch Drücken der "TEST"-Taste in der oberen rechten Ecke des Hauptmenüs aktiviert werden.

Im Testmodus können folgende Einstellungen angepasst werden:
- Touch-Invertierung (X/Y-Achsen)
- Helligkeit der Hintergrundbeleuchtung
- Touchscreen-Position testen

## Installation und Kompilierung

1. Installiere die Arduino IDE und die ESP32-Unterstützung
2. Installiere die folgenden Bibliotheken:
   - TFT_eSPI
   - XPT2046_Touchscreen
   - Adafruit GFX (wenn verwendet)
3. Stelle sicher, dass die User_Setup.h der TFT_eSPI-Bibliothek korrekt für das TZT ESP32 2.4"-Display konfiguriert ist
4. Kompiliere und lade den Code hoch

## Pin-Belegung

| Funktion | GPIO-Pin |
|----------|----------|
| TFT Data/Command | 2 |
| TFT Chip Select | 15 |
| TFT Reset | -1 |
| TFT Backlight | 27 |
| Touch Chip Select | 33 |
| Touch IRQ | 36 |
| RGB LED Red | 4 |
| RGB LED Green | 16 |
| RGB LED Blue | 17 |
| UART TX | 21 |
| UART RX | 22 |
| UART Enable | 35 |

## Debugging

Bei Problemen mit der Touch-Kalibrierung verwende den Testmodus, um die invertTouchX und invertTouchY Parameter anzupassen. Die Rohwerte und gemappten Koordinaten werden im seriellen Monitor angezeigt.

Bei Kommunikationsproblemen überprüfe:
1. START_BYTE und END_BYTE sind korrekt
2. DEVICE_ID stimmt überein
3. Alle Punkte im Telegramm-Format sind vorhanden
4. UART-Pins und Baudrate stimmen überein