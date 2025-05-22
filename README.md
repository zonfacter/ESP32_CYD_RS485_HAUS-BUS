# ESP32 Touch-Menü mit TFT Display und RS485-Kommunikation

Dieses Projekt implementiert ein interaktives Touch-Menü für TZT ESP32 2.4" LCD mit ST7789 Controller und XPT2046 Touchscreen. Es bietet ein 6-Tasten-Interface, PWM-gesteuerte Hintergrundbeleuchtung, RGB-LED-Feedback und RS485-UART-Kommunikation für industrielle Anwendungen.

## 📋 Inhaltsverzeichnis

- [Hardware-Komponenten](#hardware-komponenten)
- [Pin-Belegung](#pin-belegung)
- [RS485-TTL-Umsetzer](#rs485-ttl-umsetzer)
- [Funktionen](#funktionen)
- [Konfiguration](#konfiguration)
- [Kommunikationsprotokoll](#kommunikationsprotokoll)
- [Installation](#installation)
- [Testmodus](#testmodus)
- [Debugging](#debugging)
- [Troubleshooting](#troubleshooting)

## 🛠 Hardware-Komponenten

| Komponente | Beschreibung | Hersteller/Typ |
|------------|-------------|----------------|
| **Hauptboard** | TZT ESP32 2.4" LCD mit Touchscreen | ESP32-WROOM-32D |
| **Display** | 2.4" TFT LCD 240x320 Pixel | ST7789 Controller |
| **Touchscreen** | Resistiver Touchscreen | XPT2046 Controller |
| **Prozessor** | ESP32 Dual-Core 240MHz | Espressif ESP32 |
| **RS485-Umsetzer** | TTL zu RS485 Konverter | MAX485 oder ähnlich |
| **RGB-LED** | Status-Anzeige (optional) | Common Cathode |

## 📌 Pin-Belegung

### ESP32 TZT 2.4" LCD Board

| Funktion | GPIO-Pin | Beschreibung | Hinweise |
|----------|----------|-------------|----------|
| **Display (ST7789)** | | | |
| TFT_CS | 15 | Chip Select | Display auswählen |
| TFT_DC | 2 | Data/Command | Daten/Befehl umschalten |
| TFT_RST | -1 | Reset | Nicht verwendet (Hardware-Reset) |
| TFT_MOSI | 13 | Master Out Slave In | SPI Datenleitung |
| TFT_SCLK | 14 | Serial Clock | SPI Taktleitung |
| TFT_MISO | 12 | Master In Slave Out | SPI Rückleitung |
| TFT_BL | 27 | Backlight PWM | Hintergrundbeleuchtung |
| **Touchscreen (XPT2046)** | | | |
| T_CS | 33 | Chip Select | Touch-Controller auswählen |
| T_IRQ | 36 | Interrupt Request | Touch-Erkennung |
| T_DIN | 13 | Data In | Shared with TFT_MOSI |
| T_OUT | 12 | Data Out | Shared with TFT_MISO |
| T_CLK | 14 | Clock | Shared with TFT_SCLK |
| **UART2 (RS485)** | | | |
| UART2_RX | 21 | Receive Data | ⚠️ Hardware umverdrahtet! |
| UART2_TX | 22 | Transmit Data | ⚠️ Hardware umverdrahtet! |
| **RGB-LED (Optional)** | | | |
| LED_RED | 4 | Rote LED | Negierte Logik (LOW=Ein) |
| LED_GREEN | 16 | Grüne LED | Negierte Logik (LOW=Ein) |
| LED_BLUE | 17 | Blaue LED | Negierte Logik (LOW=Ein) |

> ⚠️ **Wichtiger Hinweis**: In dieser Hardware-Konfiguration sind RX und TX vertauscht! RX ist auf Pin 21, TX auf Pin 22.

## 🔌 RS485-TTL-Umsetzer

### Empfohlene Module
- **MAX485** - Standard RS485 Transceiver
- **SP485** - Pin-kompatible Alternative
- **SN65HVD75** - 3.3V Low-Power Variante

### Anschlussschema RS485-Umsetzer

```
ESP32 TZT Board          RS485-TTL-Umsetzer          RS485 Bus
┌─────────────────┐      ┌─────────────────────┐      ┌─────────────────┐
│                 │      │                     │      │                 │
│ GPIO22 (TX) ────┼──────┤ DI (Driver Input)   │      │                 │
│                 │      │                     │      │                 │
│ GPIO21 (RX) ────┼──────┤ RO (Receiver Out)   │      │                 │
│                 │      │                     │      │                 │
│ 3.3V VCC   ────┼──────┤ VCC                 │      │                 │
│                 │      │                     │      │                 │
│ GND        ────┼──────┤ GND                 │      │                 │
│                 │      │                     │      │                 │
│                 │      │ A/+ ────────────────┼──────┤ A/+ (Data+)     │
│                 │      │                     │      │                 │
│                 │      │ B/- ────────────────┼──────┤ B/- (Data-)     │
│                 │      │                     │      │                 │
│ N/C (Auto)  ────┼──────┤ DE (Driver Enable)  │      │                 │
│                 │      │                     │      │                 │
│ N/C (Auto)  ────┼──────┤ RE (Receiver Enable)│      │                 │
└─────────────────┘      └─────────────────────┘      └─────────────────┘
```

### RS485-Umsetzer Konfiguration

| Pin | Funktion | Verbindung | Beschreibung |
|-----|----------|------------|-------------|
| **VCC** | Spannungsversorgung | ESP32 3.3V | Betriebsspannung |
| **GND** | Ground | ESP32 GND | Masse |
| **DI** | Driver Input | ESP32 GPIO22 (TX) | Sendedaten vom ESP32 |
| **RO** | Receiver Output | ESP32 GPIO21 (RX) | Empfangsdaten zum ESP32 |
| **DE** | Driver Enable | Nicht verbunden* | Sende-Freigabe |
| **RE** | Receiver Enable | Nicht verbunden* | Empfangs-Freigabe |
| **A/+** | Data Plus | RS485 Bus A/+ | Positive Datenleitung |
| **B/-** | Data Minus | RS485 Bus B/- | Negative Datenleitung |

> *Hinweis: DE und RE werden oft zusammen verbunden oder haben interne Pull-Ups für Auto-Direction-Control.

### RS485-Bus Verkabelung

```
Gerät 1              Gerät 2              Gerät 3              Abschlusswiderstand
┌─────────┐         ┌─────────┐         ┌─────────┐         
│ A/+ ────┼─────────┼ A/+ ────┼─────────┼ A/+     │         120Ω zwischen A und B
│         │         │         │         │         │         an beiden Enden der
│ B/- ────┼─────────┼ B/-─────┼─────────┼ B/-     │         Leitungen erforderlich
│         │         │         │         │         │
│ GND ────┼─────────┼ GND ────┼─────────┼ GND     │         Gemeinsame Masse
└─────────┘         └─────────┘         └─────────┘         (optional, empfohlen)
```

## ⚡ Funktionen

### Hauptfunktionen
- **6-Tasten Touch-Interface** im 3x2 (Landscape) oder 2x3 (Portrait) Layout
- **PWM-gesteuerte Hintergrundbeleuchtung** mit 256 Stufen (0-100%)
- **RGB-LED Feedback** für visuelle Rückmeldung bei Kommunikation
- **RS485-UART-Kommunikation** für industrielle Netzwerke
- **Regelmäßige Statusmeldungen** alle 23 Sekunden
- **Touch-Kalibrierung** über integrierten Testmodus

### Button-Funktionalität
- **Visuelle Rückmeldung**: Buttons wechseln die Farbe bei Berührung
- **Zustandsspeicherung**: Aktive Buttons bleiben grün markiert
- **Protokoll-Integration**: Sendet BTN-Telegramme bei Berührung/Loslassen
- **Eindeutige IDs**: Buttons haben IDs 17-22 für das Kommunikationsprotokoll

## ⚙️ Konfiguration

### Display-Konfiguration (config.h)

```cpp
// Display-Dimensionen und Ausrichtung
#define PORTRAIT 0
#define LANDSCAPE 1
#define SCREEN_ORIENTATION LANDSCAPE  // Wechsel zu PORTRAIT für Hochformat

// Hintergrundbeleuchtungs-Pin und Kanal
#define TFT_BL_PIN 27
#define TFT_BL_CHANNEL 0  // PWM-Kanal (0-15)
#define DEFAULT_BACKLIGHT 100  // Standardhelligkeit (0-100%)
```

### RGB-LED Konfiguration (config.h)

```cpp
// RGB-LED Pins (negierte Logik: LOW = Ein, HIGH = Aus)
#define LED_RED_PIN 4      // Rote LED - Senden
#define LED_GREEN_PIN 16   // Grüne LED - Bereit
#define LED_BLUE_PIN 17    // Blaue LED - Empfangen

// LED-Blinkzeiten
#define LED_SEND_DURATION 100    // Dauer des roten Signals beim Senden (ms)
#define LED_RECEIVE_DURATION 200 // Dauer des blauen Signals beim Empfangen (ms)
```

### UART-Kommunikation (config.h)

```cpp
// UART2 Pins für RS485-Kommunikation (Hardware umverdrahtet!)
#define UART_RX_PIN 21  // RX Pin (normalerweise TX)
#define UART_TX_PIN 22  // TX Pin (normalerweise RX)
#define UART_EN_PIN -1  // Kein Enable Pin (Auto-Direction)

// Kommunikationsprotokoll Konstanten
#define START_BYTE 0xFD  // Startbyte (ý)
#define END_BYTE 0xFE    // Endbyte (þ)
#define DEVICE_ID "5999" // Eindeutige Geräte-ID
```

### Touch-Kalibrierung (config.h)

```cpp
// Touch-Parameter für Kalibrierung
#define TOUCH_MIN_X 400
#define TOUCH_MAX_X 3900
#define TOUCH_MIN_Y 400
#define TOUCH_MAX_Y 3900

// Touch-Invertierung (kann im TEST-Modus angepasst werden)
extern bool invertTouchX;  // Standardmäßig true
extern bool invertTouchY;  // Standardmäßig true
```

### Debug-Einstellungen (config.h)

```cpp
// Debug-Ausgaben aktivieren/deaktivieren
#define DB_TX_HEX 0      // Hex-Ausgabe für gesendete Telegramme
#define DB_TX_INFO 0     // Allgemeine Informationen zum Senden
#define DB_RX_HEX 1      // Hex-Ausgabe für empfangene Bytes
#define DB_RX_INFO 1     // Allgemeine Informationen zum Empfang
#define DB_INFO 0        // Allgemeine Debug-Informationen
#define RAW_DEBUG 1      // RAW-Modus für eingehende Daten
```

## 📡 Kommunikationsprotokoll

### Telegramm-Format

Alle Kommunikation erfolgt über strukturierte Telegramme:

```
[START_BYTE] DEVICE_ID.FUNCTION.INSTANCE_ID.ACTION.PARAMS [END_BYTE]
```

**Beispiel**: `ý5999.LBN.16.SET_MBR.75þ` setzt die Hintergrundbeleuchtung auf 75%.

### UART-Konfiguration

| Parameter | Wert | Beschreibung |
|-----------|------|-------------|
| **Baudrate** | 57600 | Übertragungsgeschwindigkeit |
| **Datenbits** | 8 | Anzahl Datenbits |
| **Parität** | Even (Gerade) | Fehlerprüfung |
| **Stoppbits** | 1 | Anzahl Stoppbits |
| **Format** | 8E1 | Zusammenfassung |

### Verfügbare Befehle

#### Empfangene Befehle (ESP32 als Empfänger)

| Befehl | Beschreibung | Beispiel | Antwort |
|--------|-------------|----------|---------|
| `LBN.16.SET_MBR.{0-100}` | Hintergrundbeleuchtung einstellen | `ý5999.LBN.16.SET_MBR.75þ` | Helligkeit → 75% |
| `LED.{17-22}.ON.{param}` | Button aktivieren (grün) | `ý5999.LED.17.ON.100þ` | Button 1 → grün |
| `LED.{17-22}.OFF.{param}` | Button deaktivieren (grau) | `ý5999.LED.18.OFF.0þ` | Button 2 → grau |
| `LED.{17-22}.1.{param}` | Button aktivieren (alternativ) | `ý5999.LED.19.1.0þ` | Button 3 → grün |
| `LED.{17-22}.0.{param}` | Button deaktivieren (alternativ) | `ý5999.LED.20.0.0þ` | Button 4 → grau |

#### Gesendete Befehle (ESP32 als Sender)

| Befehl | Beschreibung | Wann gesendet | Beispiel |
|--------|-------------|---------------|----------|
| `BTN.{17-22}.STATUS.1` | Button gedrückt | Bei Touch-Ereignis | `ý5999.BTN.17.STATUS.1þ` |
| `BTN.{17-22}.STATUS.0` | Button losgelassen | Nach Touch-Ereignis | `ý5999.BTN.17.STATUS.0þ` |
| `LBN.16.STATUS.{0-100}` | Helligkeitsstatus | Alle 23 Sekunden | `ý5999.LBN.16.STATUS.100þ` |

### Button-ID-Zuordnung

| Button-Position | Instanz-ID | Landscape Layout | Portrait Layout |
|----------------|------------|------------------|-----------------|
| Button 1 | 17 | Oben Links | Oben Links |
| Button 2 | 18 | Oben Mitte | Oben Rechts |
| Button 3 | 19 | Oben Rechts | Mitte Links |
| Button 4 | 20 | Unten Links | Mitte Rechts |
| Button 5 | 21 | Unten Mitte | Unten Links |
| Button 6 | 22 | Unten Rechts | Unten Rechts |

## 🚀 Installation

### Voraussetzungen

1. **Arduino IDE** (Version 1.8.19 oder höher)
2. **ESP32 Board Package** für Arduino IDE
3. **Erforderliche Bibliotheken**:
   - `TFT_eSPI` (Version 2.4.0 oder höher)
   - `XPT2046_Touchscreen`

### Bibliotheken installieren

```bash
# Arduino IDE → Tools → Manage Libraries
# Suchen und installieren:
TFT_eSPI by Bodmer
XPT2046_Touchscreen by Paul Stoffregen
```

### TFT_eSPI Konfiguration

1. Navigieren Sie zum TFT_eSPI Bibliotheksordner:
   ```
   Arduino/libraries/TFT_eSPI/
   ```

2. Öffnen Sie `User_Setup.h` und konfigurieren Sie für TZT ESP32 2.4":
   ```cpp
   #define ST7789_DRIVER
   #define TFT_WIDTH  240
   #define TFT_HEIGHT 320
   #define TFT_CS   15
   #define TFT_DC    2
   #define TFT_RST  -1
   #define TFT_BL   27
   #define TFT_MOSI 13
   #define TFT_SCLK 14
   #define TFT_MISO 12
   #define TOUCH_CS 33
   ```

### Projekt-Setup

1. **Projektverzeichnis erstellen**:
   ```
   ESP32_Touch_Menu/
   ├── ESP32_Touch_Menu.ino
   ├── config.h
   ├── communication.h
   ├── communication.cpp
   ├── menu.h
   ├── menu.cpp
   ├── touch.h
   ├── touch.cpp
   ├── display.h
   ├── display.cpp
   ├── backlight.h
   ├── backlight.cpp
   ├── led.h
   └── led.cpp
   ```

2. **Kompilieren und Hochladen**:
   - Board: "ESP32 Dev Module"
   - Upload Speed: "921600"
   - CPU Frequency: "240MHz (WiFi/BT)"
   - Flash Frequency: "80MHz"
   - Flash Size: "4MB (32Mb)"

## 🧪 Testmodus

### Testmodus aktivieren

1. Berühren Sie die **"TEST"**-Taste in der oberen rechten Ecke des Hauptmenüs
2. Der Testmodus öffnet sich mit Kalibrierungskreuzen und Kontrolltasten

### Verfügbare Testfunktionen

| Taste | Funktion | Beschreibung |
|-------|----------|-------------|
| **ESC** | Beenden | Zurück zum Hauptmenü |
| **INV-X** | X-Achse invertieren | Touchscreen-Kalibrierung X |
| **INV-Y** | Y-Achse invertieren | Touchscreen-Kalibrierung Y |
| **BL+** | Helligkeit erhöhen | +10% Hintergrundbeleuchtung |
| **BL-** | Helligkeit verringern | -10% Hintergrundbeleuchtung |

### Touch-Kalibrierung

1. **Kalibrierungskreuze antippen**: Berühren Sie die farbigen Kreuze an verschiedenen Positionen
2. **Rohwerte prüfen**: Beobachten Sie die Raw X/Y Werte im unteren Bereich
3. **Invertierung anpassen**: Nutzen Sie INV-X/INV-Y bei falscher Zuordnung
4. **Position validieren**: Überprüfen Sie die Genauigkeit der Touch-Erkennung

## 🐛 Debugging

### Debug-Ausgaben

Das System bietet verschiedene Debug-Level:

```cpp
// In config.h aktivieren/deaktivieren:
#define DB_TX_HEX 1      // Zeigt gesendete Telegramme in Hex
#define DB_RX_HEX 1      // Zeigt empfangene Bytes in Hex
#define DB_RX_INFO 1     // Zeigt Telegramm-Struktur
#define RAW_DEBUG 1      // Zeigt alle Rohdaten
```

### Typische Debug-Ausgaben

```
Kommunikation initialisiert: UART2 für RS485
- RX Pin: 21
- TX Pin: 22  
- Baudrate: 57600
- Format: 8E1

RS485 Daten verfügbar: 24 Bytes
Neues Telegramm gestartet
Telegramm vollständig empfangen
0xFD 0x35 0x39 0x39 0x39 0x2E ... 0xFE
Payload: 5999.LED.17.ON.100
DeviceID: 5999, Function: LED, InstanceID: 17, Action: ON, Params: 100
Button 1 (ID: 17) aktiviert
```

### Häufige Probleme und Lösungen

#### Touch funktioniert nicht
```cpp
// Touch-Kalibrierung prüfen in config.h:
#define TOUCH_MIN_X 400
#define TOUCH_MAX_X 3900
#define TOUCH_MIN_Y 400
#define TOUCH_MAX_Y 3900

// Touch-Invertierung im Testmodus anpassen
```

#### Keine RS485-Kommunikation
```cpp
// Hardware-Verbindungen prüfen:
ESP32 GPIO21 (RX) → RS485 RO
ESP32 GPIO22 (TX) → RS485 DI
3.3V → RS485 VCC
GND → RS485 GND

// Baudrate und Format prüfen:
SerialRS485.begin(57600, SERIAL_8E1, 21, 22);
```

#### Display zeigt nichts
```cpp
// TFT_eSPI User_Setup.h überprüfen:
#define ST7789_DRIVER
#define TFT_CS   15
#define TFT_DC    2
#define TFT_BL   27

// Hintergrundbeleuchtung aktivieren:
setBacklight(100);
```

## 🔧 Troubleshooting

### Kommunikationsprobleme

1. **RS485-Verkabelung prüfen**:
   - A/+ und B/- korrekt verbunden?
   - Abschlusswiderstände (120Ω) an beiden Enden?
   - Gemeinsame Masse zwischen allen Geräten?

2. **Baudrate validieren**:
   ```cpp
   // Testprogramm mit verschiedenen Baudraten:
   SerialRS485.begin(9600, SERIAL_8E1, 21, 22);   // Test 1
   SerialRS485.begin(57600, SERIAL_8E1, 21, 22);  // Standard
   SerialRS485.begin(115200, SERIAL_8E1, 21, 22); // Test 2
   ```

3. **Protokoll-Format überprüfen**:
   ```
   Korrekt:   ý5999.LED.17.ON.100þ
   Falsch:    5999.LED.17.ON.100    (ohne START/END Bytes)
   Falsch:    ý5999-LED-17-ON-100þ  (falsche Trennzeichen)
   ```

### Hardware-Diagnose

1. **LED-Test**:
   ```cpp
   // RGB-LED testen (negierte Logik!)
   digitalWrite(LED_RED_PIN, LOW);    // Rot ein
   digitalWrite(LED_GREEN_PIN, LOW);  // Grün ein
   digitalWrite(LED_BLUE_PIN, LOW);   // Blau ein
   ```

2. **Touchscreen-Test**:
   ```cpp
   // Raw-Werte im seriellen Monitor beobachten
   TS_Point p = touchscreen.getPoint();
   Serial.println("Raw X: " + String(p.x) + ", Raw Y: " + String(p.y));
   ```

3. **Display-Test**:
   ```cpp
   // Einfacher Farbtest
   tft.fillScreen(TFT_RED);    // Vollbild rot
   tft.fillScreen(TFT_GREEN);  // Vollbild grün
   tft.fillScreen(TFT_BLUE);   // Vollbild blau
   ```

### Performance-Optimierung

1. **Speicher-Optimierung**:
   ```cpp
   // Unnötige Debug-Ausgaben deaktivieren:
   #define DB_TX_HEX 0
   #define DB_RX_HEX 0
   #define RAW_DEBUG 0
   ```

2. **Touch-Responsivität**:
   ```cpp
   // Touch-Entprellung anpassen:
   delay(50);  // Standardwert
   delay(30);  // Schnellere Reaktion
   delay(80);  // Stabilere Erkennung
   ```

3. **Kommunikations-Timing**:
   ```cpp
   // Status-Intervall anpassen:
   #define BACKLIGHT_STATUS_INTERVAL 23000  // 23 Sekunden (Standard)
   #define BACKLIGHT_STATUS_INTERVAL 10000  // 10 Sekunden (häufiger)
   ```

## 📝 Lizenz

Dieses Projekt steht unter der MIT-Lizenz. Weitere Details finden Sie in der [LICENSE](LICENSE) Datei.

## 🤝 Beitrag

Beiträge sind willkommen! Bitte lesen Sie [CONTRIBUTING.md](CONTRIBUTING.md) für Details zum Vorgehen.

## 📞 Support

Bei Fragen oder Problemen:
1. Überprüfen Sie zuerst die [Troubleshooting](#troubleshooting)-Sektion
2. Aktivieren Sie Debug-Ausgaben für detaillierte Informationen
3. Dokumentieren Sie das Problem mit Serial Monitor Ausgaben

---

**Version**: 2.2  
**Datum**: Mai 2025  
**Kompatibilität**: ESP32, Arduino IDE 1.8.19+
